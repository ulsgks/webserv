// src/server/Server.cpp
#include "Server.hpp"

#include <errno.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>
#include <utility>

#include "../cgi/CgiManager.hpp"
#include "../config/Config.hpp"
#include "../utils/Log.hpp"
#include "../utils/Signals.hpp"
#include "Server.hpp"

// Server startup constants
static const char DEFAULT_SERVER_URL[] = "http://localhost:8080/";

// Initialize static members
std::vector<ServerBlock> Server::server_blocks_;
std::map<int, const ServerBlock*> Server::default_blocks_;
std::map<int, Socket> Server::listen_sockets_;

// ------------------------------------------------------------------
// Core server methods

Server::Server(const std::string& config_path) {
    // Load configuration
    Config::load_config(config_path, server_blocks_);
    setup_listeners();
    update_default_blocks();
}

Server::~Server() {
    // Clean up all connections
    std::vector<int> connection_fds;
    for (ConnectionMapIt it = connections_.begin(); it != connections_.end(); ++it) {
        connection_fds.push_back(it->first);
    }

    // Close and clean up each connection
    for (size_t i = 0; i < connection_fds.size(); ++i) {
        cleanup_connection(connection_fds[i]);
    }

    connections_.clear();
}

void Server::run() {
    // Log link to server for easy click access
    Log::info("Server running at " + std::string(DEFAULT_SERVER_URL));

    while (Signals::should_continue()) {
        try {
            cleanup_idle_connections();
            std::vector<PollResult> events = event_poll_.poll_once();

            for (size_t i = 0; i < events.size(); ++i) {
                const PollResult& event = events[i];

                if (process_new_connection(event)) {
                    continue;
                } else if (process_existing_connection(event)) {
                    continue;
                } else {
                    Log::warn("Unknown event on fd: " + Log::to_string(event.fd));
                }
            }
            check_cgi_processes();
        } catch (const std::exception& e) {
            Log::error("Runtime error: " + std::string(e.what()));
        }
    }
}

// ------------------------------------------------------------------
// Listeners

void Server::setup_listeners() {
    ListenPairSet unique_listeners;

    // Collect unique IP:port combinations
    for (ServerBlockVectorConstIt block = server_blocks_.begin(); block != server_blocks_.end();
         ++block) {
        for (ListenVectorConstIt listen = block->listen.begin(); listen != block->listen.end();
             ++listen) {
            unique_listeners.insert(*listen);
        }
    }

    // Setup each unique listener
    bool success = false;

    for (ListenPairSetIt it = unique_listeners.begin(); it != unique_listeners.end(); ++it) {
        try {
            setup_single_listener(it->second);
            success = true;  // At least one listener succeeded
        } catch (const std::exception& e) {
            Log::error("Failed to set up listener: " + std::string(e.what()));
        }
    }

    // If no listeners could be set up, throw an exception
    if (!success) {
        throw std::runtime_error("Failed to set up any listeners");
    }
}

void Server::setup_single_listener(int port) {
    std::pair<SocketMapIt, bool> result =
        listen_sockets_.insert(std::pair<int, Socket>(port, Socket(port)));
    Socket& socket = result.first->second;

    socket.configure_socket();
    event_poll_.watch_fd(socket.get_fd(), PollEvents::READ);
    Log::info("Listening on port " + Log::to_string(port));
}

bool Server::process_new_connection(const PollResult& event) {
    Socket* listen_socket = get_listen_socket(event.fd);
    if (!listen_socket) {
        return false;
    }

    if (event.has_error) {
        Log::error("Error on listening socket: " + Log::to_string(event.fd));
    } else if (event.can_read) {
        handle_new_connection(listen_socket);
    }
    return true;
}

Socket* Server::get_listen_socket(int fd) const {
    for (SocketMapConstIt it = listen_sockets_.begin(); it != listen_sockets_.end(); ++it) {
        if (fd == it->second.get_fd()) {
            return const_cast<Socket*>(&it->second);
        }
    }
    return NULL;
}

void Server::handle_new_connection(Socket* listen_socket) {
    int client_fd = listen_socket->accept_connection();
    if (client_fd >= 0) {
        Log::info("New connection accepted (fd: " + Log::to_string(client_fd) + ")");
        Connection* conn = new Connection(client_fd, event_poll_);

        // Find the port associated with this listening socket
        int port = -1;
        for (SocketMapConstIt it = listen_sockets_.begin(); it != listen_sockets_.end(); ++it) {
            if (it->second.get_fd() == listen_socket->get_fd()) {
                port = it->first;
                break;
            }
        }

        // Set default server block for this port
        if (port != -1) {
            DefaultBlockMapConstIt it = default_blocks_.find(port);
            if (it != default_blocks_.end()) {
                conn->set_server_block(it->second);
            }
        }

        connections_[client_fd] = conn;
    }
}

// ------------------------------------------------------------------
// Connection management

bool Server::process_existing_connection(const PollResult& event) {
    ConnectionMapIt conn_it = connections_.find(event.fd);
    if (conn_it == connections_.end()) {
        // Check if this might be a CGI I/O fd
        if (process_cgi_output(event)) {
            return true;
        }
        return process_cgi_input(event);
    }

    Connection* conn = conn_it->second;
    if (event.has_error) {
        conn->close_on_error();
    } else if (event.can_read) {
        conn->receive_client_data();
    } else if (event.can_write) {
        conn->send_response_data();
    }
    if (conn->should_close()) {
        cleanup_connection(event.fd);
    }
    return true;
}

bool Server::process_cgi_output(const PollResult& event) {
    // Find connection with matching CGI stdout_fd
    for (ConnectionMapIt it = connections_.begin(); it != connections_.end(); ++it) {
        Connection* conn = it->second;
        if (conn->is_cgi_active()) {
            // Use CgiManager to process the output
            if (conn->get_cgi_manager().process_cgi_output(event.fd, conn, event_poll_, event)) {
                return true;
            }
        }
    }
    return false;
}

bool Server::process_cgi_input(const PollResult& event) {
    // Find connection with matching CGI stdin_fd
    for (ConnectionMapIt it = connections_.begin(); it != connections_.end(); ++it) {
        Connection* conn = it->second;
        if (conn->is_cgi_active()) {
            // Use CgiManager to process the input
            if (conn->get_cgi_manager().process_cgi_input(event.fd, conn, event_poll_, event)) {
                return true;
            }
        }
    }
    return false;
}

void Server::cleanup_idle_connections() {
    time_t current_time = time(NULL);

    // Check for idle connections and let them handle timeout response
    for (ConnectionMapIt it = connections_.begin(); it != connections_.end(); ++it) {
        it->second->is_idle(current_time);  // This will send 408 if timeout
    }

    // Then clean up connections that should be closed
    std::vector<int> to_close;
    for (ConnectionMapIt it = connections_.begin(); it != connections_.end(); ++it) {
        if (it->second->should_close()) {
            to_close.push_back(it->first);
        }
    }

    for (size_t i = 0; i < to_close.size(); ++i) {
        cleanup_connection(to_close[i]);
    }
}

void Server::cleanup_connection(int fd) {
    ConnectionMapIt it = connections_.find(fd);
    if (it != connections_.end()) {
        delete it->second;
        connections_.erase(it);
    }
}

// ------------------------------------------------------------------
// Server block management

void Server::update_default_blocks() {
    default_blocks_.clear();

    // For each port, find the default server block
    for (SocketMapConstIt socket = listen_sockets_.begin(); socket != listen_sockets_.end();
         ++socket) {
        int port = socket->first;
        const ServerBlock* default_block = NULL;

        // Find default server for this port
        for (ServerBlockVectorConstIt block = server_blocks_.begin(); block != server_blocks_.end();
             ++block) {
            for (ListenVectorConstIt listen = block->listen.begin(); listen != block->listen.end();
                 ++listen) {
                if (listen->second == port) {
                    if (block->is_default || default_block == NULL) {
                        default_block = &(*block);
                    }
                }
            }
        }

        if (default_block) {
            default_blocks_[port] = default_block;
        }
    }
}

// Static method implementation
const ServerBlock* Server::get_server_block(const std::string& host, int port) {
    // First try to find a server matching both host and port
    for (ServerBlockVectorConstIt it = server_blocks_.begin(); it != server_blocks_.end(); ++it) {
        for (ListenVectorConstIt listen = it->listen.begin(); listen != it->listen.end();
             ++listen) {
            if (listen->second == port && it->matches_server_name(host)) {
                return &(*it);
            }
        }
    }

    // If no match, return the default server for this port
    DefaultBlockMapConstIt it = default_blocks_.find(port);
    return (it != default_blocks_.end()) ? it->second : NULL;
}

void Server::check_cgi_processes() {
    // Iterate through all connections and check for active CGI processes
    for (ConnectionMapIt it = connections_.begin(); it != connections_.end(); ++it) {
        Connection* connection = it->second;
        CgiManager& cgi_manager = connection->get_cgi_manager();

        if (cgi_manager.is_cgi_active(connection)) {
            cgi_manager.update_cgi_process(connection, event_poll_);
        }
    }
}

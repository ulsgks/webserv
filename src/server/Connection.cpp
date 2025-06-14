#include "Connection.hpp"

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "../http/handler/Handler.hpp"
#include "../utils/Log.hpp"
#include "Server.hpp"
#include "Socket.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

// Connection constants
static const char LOCALHOST_IP[] = "127.0.0.1";
static const int FALLBACK_SERVER_PORT = 8080;

// Static member definitions
const size_t Connection::MAX_REQUESTS;
const time_t Connection::TIMEOUT;
const size_t Connection::BUFFER_SIZE;

// Constructor
Connection::Connection(int client_fd, EventPoller& poller)
    : fd_(client_fd),
      poller_(poller),
      last_activity_(time(NULL)),
      should_close_(false),
      request_count_(0),
      request_in_progress_(false),
      server_block_(NULL) {
    // Register with poller for initial read events
    poller_.watch_fd(fd_, PollEvents::READ);
}

// Destructor
Connection::~Connection() {
    // Clean up any active CGI process first
    cgi_manager_.cleanup_cgi_process(this, poller_);

    if (fd_ != -1) {
        // Unregister from event polling
        poller_.unwatch_fd(fd_);
        // Close the socket
        close(fd_);
        fd_ = -1;
    }
}

void Connection::receive_client_data() {
    // Don't receive if we're marked for closing
    if (should_close_) {
        return;
    }

    try {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = recv(fd_, buffer, BUFFER_SIZE, 0);

        if (bytes_read > 0) {
            // Data received successfully
            std::string data(buffer, bytes_read);
            update_activity_time();

            // If no request is in progress, create a new one
            if (!request_in_progress_) {
                current_request_.reset();
                request_in_progress_ = true;
            }

            // Pass the new data to the request for parsing
            current_request_.append_data(data);

            // If request is complete, process it
            if (current_request_.is_complete()) {
                handle_http_request();
                request_in_progress_ = false;
            }
        } else if (bytes_read == 0) {
            // Client closed connection
            should_close_ = true;
        } else {
            // recv() returned -1, could be EAGAIN/EWOULDBLOCK (expected) or real error
            // Subject forbids checking errno, so we handle this gracefully
            // For non-blocking sockets, this is expected when no data is available
            Log::warn(
                "CONNECTION: fd=" + Log::to_string(fd_) +
                " recv() returned -1 (expected for non-blocking)");
        }

        // Update poll events based on buffer states
        short events = PollEvents::READ;
        if (!response_buffer_.empty()) {
            events |= PollEvents::WRITE;
        }
        update_events(events);
    } catch (const HttpError& e) {
        handle_http_error(e);
    } catch (const std::exception& e) {
        handle_http_error(HttpError(INTERNAL_SERVER_ERROR, e.what()));
    }
}

void Connection::send_response_data() {
    // Nothing to send
    if (response_buffer_.empty()) {
        return;
    }

    try {
        ssize_t bytes_sent = send(fd_, response_buffer_.c_str(), response_buffer_.length(), 0);

        if (bytes_sent > 0) {
            // Update activity time and remove sent data from buffer
            update_activity_time();
            response_buffer_ = response_buffer_.substr(bytes_sent);

            // Update poll events
            short events = PollEvents::READ;
            if (!response_buffer_.empty()) {
                events |= PollEvents::WRITE;
            }
            update_events(events);
        } else if (bytes_sent == -1) {
            // send() returned -1, could be EAGAIN/EWOULDBLOCK (expected) or real error
            // Subject forbids checking errno, so we handle this gracefully
            // For non-blocking sockets, this is expected when write would block
            Log::warn(
                "CONNECTION: fd=" + Log::to_string(fd_) +
                " send() returned -1 (expected for non-blocking)");
        }
    } catch (const HttpError& e) {
        handle_http_error(e);
    } catch (const std::exception& e) {
        handle_http_error(HttpError(INTERNAL_SERVER_ERROR, e.what()));
    }
}

void Connection::close_on_error() {
    Log::error("Error on connection " + Log::to_string(fd_));
    should_close_ = true;
}

bool Connection::should_close() const {
    // Only close when marked AND all pending data has been sent
    return should_close_ && response_buffer_.empty();
}

bool Connection::is_idle(time_t current_time) const {
    time_t idle_time = current_time - last_activity_;
    bool is_timeout = idle_time > TIMEOUT;

    if (is_timeout) {
        Log::warn(
            "Connection " + Log::to_string(fd_) + " has been idle for " +
            Log::to_string(idle_time) + " seconds, timing out.");
        // Launch a timeout response
        Connection* mutable_this = const_cast<Connection*>(this);
        try {
            mutable_this->send_timeout_response();
        } catch (const HttpError& e) {
            Log::error("Failed to send timeout response: " + std::string(e.what()));
            mutable_this->handle_http_error(e);
        }
    }

    return is_timeout;
}

void Connection::set_server_block(const ServerBlock* block) {
    server_block_ = block;
}

void Connection::select_server_block_for_request() {
    // Get the host header from the request
    std::string host = current_request_.get_header(HttpHeaders::HOST);

    // If no host header and HTTP/1.0, use default
    if (host.empty() && current_request_.get_http_version() == "HTTP/1.0") {
        host = "localhost";  // Fallback for HTTP/1.0 without Host
        return;              // Keep existing server_block_
    }

    // Extract port from host if present (host:port format)
    int port = -1;
    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        std::string port_str = host.substr(colon_pos + 1);
        host = host.substr(0, colon_pos);
        port = std::atoi(port_str.c_str());
    } else {
        // Try to determine port from the listening socket
        for (SocketMapConstIt it = Server::get_listen_sockets().begin();
             it != Server::get_listen_sockets().end(); ++it) {
            if (it->second.get_fd() == fd_) {
                port = it->first;
                break;
            }
        }
    }

    // Update server block based on host and port if possible
    if (!host.empty() && port != -1) {
        const ServerBlock* matched_block = Server::get_server_block(host, port);
        if (matched_block) {
            server_block_ = matched_block;
        }
    }
}

void Connection::handle_http_request() {
    // Select the appropriate server block based on the request's Host header
    select_server_block_for_request();

    // Verify we have a server block configuration
    if (!server_block_) {
        handle_http_error(HttpError(INTERNAL_SERVER_ERROR, "No server configuration available"));
        return;
    }

    Log::info(
        HttpMethods::to_string(current_request_.get_method()) + " " + current_request_.get_path());
    Log::debug(current_request_);

    // Process the request and get a response, passing the server_block
    HttpHandler handler;
    HttpResponse response = handler.handle_request(current_request_, *server_block_, this);

    // Check if CGI is in progress (special header)
    if (response.get_header("X-CGI-Processing") == "true") {
        return;  // Don't send response yet, CGI will handle it later
    }

    Log::debug(response);

    // Decide on connection persistence using HttpRequest's method
    bool keep_alive = current_request_.is_keep_alive();

    // Set Connection header appropriately
    if (keep_alive) {
        response.set_header(HttpHeaders::CONNECTION, "keep-alive");
    } else {
        response.set_header(HttpHeaders::CONNECTION, "close");
        should_close_ = true;
    }

    // Increment request count and check if we've hit the limit
    request_count_++;
    if (request_count_ >= MAX_REQUESTS) {
        should_close_ = true;
    }

    // Build the response and store in buffer
    response_buffer_ = response.build();

    // Update poll events to include writing
    update_events(PollEvents::READ | PollEvents::WRITE);
}

void Connection::handle_http_error(const HttpError& error) {
    Log::error(
        "HTTP error on fd " + Log::to_string(fd_) + ": " + Log::to_string(error.get_status_code()) +
        " " + error.get_status_message());

    try {
        // Create an HTTP response from the error using the utility method
        HttpResponse response = HttpResponse::build_default_error_response(error);

        // NGINX-like behavior: Only close for certain errors
        if (error.should_close_connection()) {
            // For serious errors, always close
            should_close_ = true;
        } else if (request_in_progress_) {
            // For less serious errors, respect the client's keep-alive preference if we have a
            // request
            bool keep_alive = current_request_.is_keep_alive();
            if (!keep_alive) {
                should_close_ = true;
            }

            // Set appropriate Connection header based on decision
            if (keep_alive && !should_close_) {
                response.set_header(HttpHeaders::CONNECTION, "keep-alive");
            } else {
                response.set_header(HttpHeaders::CONNECTION, "close");
            }
        }

        // Set the response buffer
        response_buffer_ = response.build();

        // Update poll events to include writing
        update_events(PollEvents::WRITE);
    } catch (const std::exception& e) {
        // Critical failure, just mark for closing
        Log::error("Failed to create error response: " + std::string(e.what()));
        should_close_ = true;
    }
}

// Get client IP address
std::string Connection::get_client_ip() const {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(fd_, (struct sockaddr*)&client_addr, &addr_len) == 0) {
        char ip_str[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN)) {
            return std::string(ip_str);
        }
    }

    // Fallback to localhost if getpeername fails
    return LOCALHOST_IP;
}

// Get client hostname (for now, return IP as hostname)
std::string Connection::get_client_host() const {
    return get_client_ip();
}

// Get server port from the server block configuration
int Connection::get_server_port() const {
    if (server_block_) {
        const ListenVector& listen_pairs = server_block_->listen;
        if (!listen_pairs.empty()) {
            return listen_pairs[0].second;  // Return the port from the first listen directive
        }
    }

    // Fallback: try to get port from socket
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    if (getsockname(fd_, (struct sockaddr*)&server_addr, &addr_len) == 0) {
        return ntohs(server_addr.sin_port);
    }

    // Final fallback
    return FALLBACK_SERVER_PORT;
}

void Connection::update_activity_time() {
    last_activity_ = time(NULL);
}

void Connection::update_events(short events) {
    poller_.update_events(fd_, events);
}

void Connection::send_timeout_response() {
    throw HttpError(REQUEST_TIMEOUT, "Request Timeout");
}

// CGI handling methods
bool Connection::start_cgi_execution(
    const HttpRequest& request, const std::string& script_path, const LocationBlock* location) {
    return cgi_manager_.start_cgi_execution(request, script_path, location, this, poller_);
}

void Connection::handle_cgi_completion() {
    cgi_manager_.handle_cgi_completion(this, poller_);
}

void Connection::handle_cgi_timeout() {
    cgi_manager_.handle_cgi_timeout(this, poller_);
}

void Connection::cleanup_cgi_process() {
    cgi_manager_.cleanup_cgi_process(this, poller_);
}

// Methods for CgiManager to access connection internals
void Connection::set_response_from_cgi(const std::string& response_data) {
    response_buffer_ = response_data;
    // Update events to include writing
    update_events(PollEvents::READ | PollEvents::WRITE);
}

void Connection::send_error_response(HttpStatusCode status, const std::string& message) {
    HttpError error(status, message);
    handle_http_error(error);
}

bool Connection::is_cgi_active() const {
    return cgi_manager_.is_cgi_active(this);
}

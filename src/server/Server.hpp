#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "../config/contexts/ServerBlock.hpp"
#include "../utils/Types.hpp"
#include "Connection.hpp"
#include "EventPoller.hpp"
#include "Socket.hpp"

class Server {
   private:
    static ServerBlockVector server_blocks_;  // Store server blocks
    static DefaultBlockMap default_blocks_;   // Quick lookup for default blocks per port
    static SocketMap listen_sockets_;         // Sockets by port
    EventPoller event_poll_;                  // Event loop helper
    ConnectionMap connections_;               // Active connections
    void check_cgi_processes();

   public:
    explicit Server(const std::string& config_path);
    ~Server();

    // Prevent copying
    Server(const Server& other);
    Server& operator=(const Server& other);

    // Server lifecycle
    void run();

    // Static methods for Connection class to use
    static const ServerBlock* get_server_block(const std::string& host, int port);
    static SocketMap& get_listen_sockets() {
        return listen_sockets_;
    }

    // Added for completeness - allows accessing server blocks from other components if needed
    static const ServerBlockVector& get_server_blocks() {
        return server_blocks_;
    }

   private:
    // Listener management
    void setup_listeners();
    void setup_single_listener(int port);
    bool process_new_connection(const PollResult& event);
    Socket* get_listen_socket(int fd) const;
    void handle_new_connection(Socket* listen_socket);

    // Connection management
    bool process_existing_connection(const PollResult& event);
    bool process_cgi_output(const PollResult& event);
    bool process_cgi_input(const PollResult& event);
    void cleanup_idle_connections();
    void cleanup_connection(int fd);

    // Server block management
    void update_default_blocks();
};

#endif  // SERVER_HPP

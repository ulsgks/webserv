#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <ctime>
#include <string>

#include "../cgi/CgiManager.hpp"
#include "../config/contexts/ServerBlock.hpp"
#include "../http/error/Error.hpp"
#include "../http/request/Request.hpp"
#include "../http/response/Response.hpp"
#include "../utils/Types.hpp"
#include "EventPoller.hpp"

/**
 * Manages a client connection, handling request/response lifecycle.
 *
 * The Connection class is responsible for:
 * - Managing the socket connection with a client
 * - Reading data from the client and passing it to HttpRequest
 * - Processing HTTP requests and generating responses
 * - Sending responses back to the client
 * - Managing connection lifecycle (keep-alive, timeouts, errors)
 */
class Connection {
   public:
    // Constructor takes client socket fd and reference to event poller
    Connection(int client_fd, EventPoller& poller);

    // Destructor ensures socket cleanup
    ~Connection();

    void receive_client_data();
    void send_response_data();
    void close_on_error();
    bool should_close() const;
    bool is_idle(time_t current_time) const;
    int get_fd() const {
        return fd_;
    }
    void set_server_block(const ServerBlock* block);

    // Methods for CGI environment
    std::string get_client_ip() const;
    std::string get_client_host() const;
    int get_server_port() const;

    // CGI integration with CgiManager
    bool start_cgi_execution(
        const HttpRequest& request, const std::string& path, const LocationBlock* location);
    void handle_cgi_completion();
    void handle_cgi_timeout();
    void cleanup_cgi_process();

    // Methods for CgiManager to access connection internals
    void set_response_from_cgi(const std::string& response_data);
    void send_error_response(HttpStatusCode status, const std::string& message);

    // CGI state access (for CgiManager)
    bool is_cgi_active() const;
    CgiManager& get_cgi_manager() {
        return cgi_manager_;
    }

   private:
    // Connection constants
    static const size_t MAX_REQUESTS = 100;   // Maximum requests per connection
    static const time_t TIMEOUT = 60;         // Connection timeout in seconds
    static const size_t BUFFER_SIZE = 32768;  // Read buffer size (32KB)
    // Buffer size recommendations:
    // 8KB (8192 bytes): A common default that works well for most HTTP servers
    // 4KB (4096 bytes): Minimum reasonable size for most HTTP operations
    // 16KB-32KB: Good for high-performance scenarios with available memory
    // Buffer size recommendations:
    // 8KB (8192 bytes): A common default that works well for most HTTP servers
    // 4KB (4096 bytes): Minimum reasonable size for most HTTP operations
    // 16KB-32KB: Good for high-performance scenarios with available memory

    // Socket information
    int fd_;               // Client socket file descriptor
    EventPoller& poller_;  // Reference to the event poller

    // Connection state
    time_t last_activity_;  // Timestamp of last activity
    bool should_close_;     // Flag indicating if connection should be closed
    size_t request_count_;  // Number of requests processed on this connection

    // Request/response state
    std::string response_buffer_;  // Buffer for outgoing response data
    HttpRequest current_request_;  // Current HTTP request being processed
    bool request_in_progress_;     // Flag indicating if a request is being processed

    // Configuration
    const ServerBlock* server_block_;  // Server configuration for this connection

    // CGI management
    CgiManager cgi_manager_;  // Manages CGI processes for this connection

    void handle_http_request();
    void send_timeout_response();
    void handle_http_error(const HttpError& error);
    void select_server_block_for_request();

    void update_activity_time();
    void update_events(short events);

    // Prevent copying
    Connection(const Connection&);
    Connection& operator=(const Connection&);
};

#endif  // CONNECTION_HPP

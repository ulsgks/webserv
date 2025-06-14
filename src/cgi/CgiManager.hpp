#ifndef CGI_MANAGER_HPP
#define CGI_MANAGER_HPP

#include <ctime>
#include <map>
#include <string>

#include "../cgi/CgiEnvironment.hpp"
#include "../cgi/CgiProcess.hpp"
#include "../cgi/CgiResponse.hpp"
#include "../config/contexts/LocationBlock.hpp"
#include "../http/error/Error.hpp"
#include "../http/request/Request.hpp"
#include "../http/response/Response.hpp"
#include "../server/EventPoller.hpp"
#include "../utils/Log.hpp"
#include "../utils/Types.hpp"
#include <sys/types.h>

// Forward declarations
class Connection;

/**
 * Manages CGI process execution and lifecycle.
 *
 * The CgiManager class is responsible for:
 * - Starting and managing CGI processes
 * - Handling CGI output and completion
 * - Managing CGI timeouts
 * - Cleaning up CGI resources
 */
class CgiManager {
   public:
    /**
     * Represents the state of a CGI process.
     */
    struct CgiState {
        bool active;
        pid_t pid;
        int stdout_fd;
        int stdin_fd;
        time_t start_time;
        std::string accumulated_output;
        HttpRequest cgi_request;
        const LocationBlock* location;
        size_t request_body_sent;  // Track how much of the request body has been sent

        CgiState()
            : active(false),
              pid(-1),
              stdout_fd(-1),
              stdin_fd(-1),
              start_time(0),
              location(NULL),
              request_body_sent(0) {
        }
    };

    CgiManager();
    ~CgiManager();

    // Core CGI functionality
    bool start_cgi_execution(
        const HttpRequest& request, const std::string& script_path, const LocationBlock* location,
        Connection* connection, EventPoller& poller);

    bool handle_cgi_completion(Connection* connection, EventPoller& poller);
    bool handle_cgi_timeout(Connection* connection, EventPoller& poller);

    // Convenience method that checks completion first, then timeout
    void update_cgi_process(Connection* connection, EventPoller& poller);

    void cleanup_cgi_process(Connection* connection, EventPoller& poller);

    // CGI I/O processing
    bool process_cgi_output(
        int cgi_fd, Connection* connection, EventPoller& poller, const PollResult& event);
    bool process_cgi_input(
        int cgi_fd, Connection* connection, EventPoller& poller, const PollResult& event);

    // CGI state management
    bool is_cgi_active(const Connection* connection) const;
    const CgiState& get_cgi_state(const Connection* connection) const;
    CgiState& get_cgi_state(Connection* connection);

   private:
    // CGI execution constants
    static const int CGI_TIMEOUT_SECONDS = 5;    // CGI process timeout in seconds
    static const size_t CGI_BUFFER_SIZE = 8192;  // Buffer size for reading CGI output

    // Map to store CGI state for each connection
    std::map<Connection*, CgiState> cgi_states_;

    // Helper methods
    void reset_cgi_state(Connection* connection);
    void send_cgi_error_response(
        Connection* connection, HttpStatusCode status, const std::string& message);
    static std::string find_interpreter(
        const std::string& script_path, const LocationBlock& location);

    // Prevent copying
    CgiManager(const CgiManager&);
    CgiManager& operator=(const CgiManager&);
};

#endif  // CGI_MANAGER_HPP

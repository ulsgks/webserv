#include "CgiManager.hpp"

#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "../server/Connection.hpp"
#include "../server/EventPoller.hpp"
#include <sys/wait.h>

CgiManager::CgiManager() {
}

CgiManager::~CgiManager() {
    // Clean up any remaining CGI states
    for (std::map<Connection*, CgiState>::iterator it = cgi_states_.begin();
         it != cgi_states_.end(); ++it) {
        if (it->second.active) {
            Log::warn("Cleaning up active CGI process during CgiManager destruction");
            // Kill the process if still running
            if (it->second.pid > 0) {
                kill(it->second.pid, SIGKILL);
                waitpid(it->second.pid, NULL, 0);
            }
        }
    }
    cgi_states_.clear();
}

bool CgiManager::start_cgi_execution(
    const HttpRequest& request, const std::string& script_path, const LocationBlock* location,
    Connection* connection, EventPoller& poller) {
    // Check if CGI is already active for this connection
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it != cgi_states_.end() && it->second.active) {
        return false;  // CGI already in progress
    }

    try {
        Log::info("Starting CGI: " + script_path);

        // Find interpreter and build environment
        std::string interpreter = find_interpreter(script_path, *location);

        // Get connection information for CGI environment
        int server_port = connection->get_server_port();
        std::string client_ip = connection->get_client_ip();
        std::string client_host = connection->get_client_host();

        std::vector<std::string> env_vector = CgiEnvironment::build(
            request, script_path, *location, server_port, client_ip, client_host);

        // Start non-blocking CGI execution
        pid_t pid;
        int stdout_fd, stdin_fd;

        if (!CgiProcess::start_execution(
                interpreter, script_path, env_vector, request.get_body(), pid, stdout_fd,
                stdin_fd)) {
            return false;
        }

        // Create or update CGI state for this connection
        CgiState& cgi_state = cgi_states_[connection];
        cgi_state.active = true;
        cgi_state.pid = pid;
        cgi_state.stdout_fd = stdout_fd;
        cgi_state.stdin_fd = stdin_fd;
        cgi_state.start_time = time(NULL);
        cgi_state.cgi_request = request;
        cgi_state.location = location;
        cgi_state.accumulated_output.clear();

        // Add stdout_fd to event polling for reading
        poller.watch_fd(stdout_fd, PollEvents::READ);

        // If we have a request body and stdin is still open, watch stdin for writing
        if (stdin_fd != -1 && !request.get_body().empty()) {
            poller.watch_fd(stdin_fd, PollEvents::WRITE);
        }

        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool CgiManager::handle_cgi_completion(Connection* connection, EventPoller& poller) {
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it == cgi_states_.end() || !it->second.active) {
        return false;
    }

    CgiState& cgi_state = it->second;

    // Check if process is complete
    int status;
    pid_t result = waitpid(cgi_state.pid, &status, WNOHANG);

    if (result == cgi_state.pid) {
        // Process completed
        close(cgi_state.stdout_fd);
        poller.unwatch_fd(cgi_state.stdout_fd);

        // Check process exit status before building response
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            Log::error(
                "CGI process exited abnormally with status: " +
                Log::to_string(WEXITSTATUS(status)));

            // Reset CGI state
            reset_cgi_state(connection);

            // Send 500 Internal Server Error
            send_cgi_error_response(
                connection, INTERNAL_SERVER_ERROR, "CGI script execution failed");
            return true;
        }

        // Build response from accumulated output
        try {
            HttpResponse response = CgiResponse::build_from_output(cgi_state.accumulated_output);

            // Set the response on the connection for sending to client
            connection->set_response_from_cgi(response.build());

        } catch (const std::exception& e) {
            Log::error("Failed to build CGI response: " + std::string(e.what()));
            send_cgi_error_response(
                connection, INTERNAL_SERVER_ERROR, "Failed to process CGI output");
        }

        // Reset CGI state
        reset_cgi_state(connection);
        return true;
    }

    return false;
}

bool CgiManager::handle_cgi_timeout(Connection* connection, EventPoller& poller) {
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it == cgi_states_.end() || !it->second.active) {
        return false;
    }

    CgiState& cgi_state = it->second;
    time_t current_time = time(NULL);

    if (current_time - cgi_state.start_time >= CGI_TIMEOUT_SECONDS) {
        // Send timeout error response before cleanup
        send_cgi_error_response(connection, GATEWAY_TIMEOUT, "CGI script timeout");

        // Clean up the CGI process
        cleanup_cgi_process(connection, poller);

        Log::warn("CGI execution timed out for connection");
        return true;
    }

    return false;
}

void CgiManager::update_cgi_process(Connection* connection, EventPoller& poller) {
    // Try completion first, then timeout if not completed
    if (!handle_cgi_completion(connection, poller)) {
        handle_cgi_timeout(connection, poller);
    }
}

void CgiManager::cleanup_cgi_process(Connection* connection, EventPoller& poller) {
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it == cgi_states_.end() || !it->second.active) {
        return;
    }

    CgiState& cgi_state = it->second;

    Log::warn(
        "Cleaning up active CGI process (pid: " + Log::to_string(cgi_state.pid) +
        ") due to connection close or timeout");

    // Kill the CGI process
    if (cgi_state.pid > 0) {
        kill(cgi_state.pid, SIGKILL);
        waitpid(cgi_state.pid, NULL, 0);
    }

    // Cleanup CGI file descriptors
    if (cgi_state.stdout_fd != -1) {
        close(cgi_state.stdout_fd);
        poller.unwatch_fd(cgi_state.stdout_fd);
    }
    if (cgi_state.stdin_fd != -1) {
        close(cgi_state.stdin_fd);
    }

    // Reset CGI state
    reset_cgi_state(connection);
}

bool CgiManager::process_cgi_output(
    int cgi_fd, Connection* connection, EventPoller& poller, const PollResult& event) {
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it == cgi_states_.end() || !it->second.active || it->second.stdout_fd != cgi_fd) {
        return false;
    }

    CgiState& cgi_state = it->second;

    if (event.can_read) {
        // Read CGI output
        char buffer[CGI_BUFFER_SIZE];
        ssize_t bytes_read = read(cgi_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            cgi_state.accumulated_output.append(buffer, bytes_read);
        } else if (bytes_read == 0) {
            // EOF - CGI process finished
            handle_cgi_completion(connection, poller);
        } else {
            // bytes_read < 0 - could be EAGAIN/EWOULDBLOCK (expected) or real error
            // Subject forbids checking errno, so we handle this gracefully
            // For non-blocking sockets, this is expected when no data is available
            Log::warn("CGI read() returned -1 (expected for non-blocking fd)");
            // Just ignore and wait for next event - poll will notify us again if needed
        }
    } else if (event.has_error) {
        Log::error("Error event on CGI fd: " + Log::to_string(cgi_fd));
        handle_cgi_completion(connection, poller);
    }

    return true;
}

bool CgiManager::process_cgi_input(
    int cgi_fd, Connection* connection, EventPoller& poller, const PollResult& event) {
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it == cgi_states_.end() || !it->second.active || it->second.stdin_fd != cgi_fd) {
        return false;
    }

    CgiState& cgi_state = it->second;

    if (event.can_write) {
        // Send request body data to CGI stdin
        const std::string& request_body = cgi_state.cgi_request.get_body();

        if (cgi_state.request_body_sent < request_body.length()) {
            // Send remaining data
            size_t remaining = request_body.length() - cgi_state.request_body_sent;
            const char* data = request_body.c_str() + cgi_state.request_body_sent;

            ssize_t written = write(cgi_fd, data, remaining);
            if (written > 0) {
                cgi_state.request_body_sent += written;

                // Check if we've sent all the data
                if (cgi_state.request_body_sent >= request_body.length()) {
                    // All data sent, close stdin and stop watching it
                    close(cgi_fd);
                    poller.unwatch_fd(cgi_fd);
                    cgi_state.stdin_fd = -1;
                }
            } else if (written == 0) {
                // Pipe closed unexpectedly
                close(cgi_fd);
                poller.unwatch_fd(cgi_fd);
                cgi_state.stdin_fd = -1;
            } else {
                // written < 0 - could be EAGAIN/EWOULDBLOCK (expected) or real error
                // Subject forbids checking errno, so we handle this gracefully
                Log::warn("CGI write() returned -1 (expected for non-blocking fd)");
                // Just ignore and wait for next event - poll will notify us again if needed
            }
        } else {
            // All data already sent, close stdin
            close(cgi_fd);
            poller.unwatch_fd(cgi_fd);
            cgi_state.stdin_fd = -1;
        }
    } else if (event.has_error) {
        Log::error("Error event on CGI stdin fd: " + Log::to_string(cgi_fd));
        close(cgi_fd);
        poller.unwatch_fd(cgi_fd);
        cgi_state.stdin_fd = -1;
    }

    return true;
}

bool CgiManager::is_cgi_active(const Connection* connection) const {
    std::map<Connection*, CgiState>::const_iterator it =
        cgi_states_.find(const_cast<Connection*>(connection));
    return it != cgi_states_.end() && it->second.active;
}

const CgiManager::CgiState& CgiManager::get_cgi_state(const Connection* connection) const {
    std::map<Connection*, CgiState>::const_iterator it =
        cgi_states_.find(const_cast<Connection*>(connection));
    if (it == cgi_states_.end()) {
        static const CgiState empty_state;
        return empty_state;
    }
    return it->second;
}

CgiManager::CgiState& CgiManager::get_cgi_state(Connection* connection) {
    return cgi_states_[connection];
}

void CgiManager::reset_cgi_state(Connection* connection) {
    std::map<Connection*, CgiState>::iterator it = cgi_states_.find(connection);
    if (it != cgi_states_.end()) {
        it->second.active = false;
        it->second.pid = -1;
        it->second.stdout_fd = -1;
        it->second.stdin_fd = -1;
        it->second.start_time = 0;
        it->second.location = NULL;
        it->second.accumulated_output.clear();
        // We could erase the entry entirely, but keeping it allows for potential reuse
        // cgi_states_.erase(it);
    }
}

void CgiManager::send_cgi_error_response(
    Connection* connection, HttpStatusCode status, const std::string& message) {
    try {
        // Use the public method designed for CGI error responses
        connection->send_error_response(status, message);
    } catch (const std::exception& e) {
        Log::error("Failed to send CGI error response: " + std::string(e.what()));
    }
}

std::string CgiManager::find_interpreter(
    const std::string& script_path, const LocationBlock& location) {
    // Extract file extension
    size_t dot_pos = script_path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        throw HttpError(INTERNAL_SERVER_ERROR, "CGI script has no extension");
    }

    std::string extension = script_path.substr(dot_pos);

    // Handle .cgi files as direct executables (no interpreter needed)
    if (extension == ".cgi") {
        return "";  // Empty string indicates direct execution
    }

    // Look up handler for other extensions
    CgiHandlerMapConstIt it = location.cgi_handlers.find(extension);
    if (it == location.cgi_handlers.end()) {
        throw HttpError(
            INTERNAL_SERVER_ERROR, "No CGI handler configured for extension: " + extension);
    }

    return it->second;
}

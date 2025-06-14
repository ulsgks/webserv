#include <unistd.h>

#include <utility>

#include "../../server/Connection.hpp"
#include "../../utils/Log.hpp"
#include "Handler.hpp"
#include <sys/stat.h>

bool HttpHandler::is_cgi_request(const std::string& path, const LocationBlock* location) const {
    if (!location || !location->cgi_enabled) {
        return false;
    }

    // For CGI requests, we need to check if the path starts with a CGI script
    // The path might include PATH_INFO after the script name

    // First, try to find a CGI script by checking each path segment
    std::string current_path = "";
    size_t start = 0;

    while (start < path.length()) {
        size_t next_slash = path.find('/', start);
        if (next_slash == std::string::npos) {
            current_path = path;
        } else {
            current_path = path.substr(0, next_slash);
        }

        // Check if this path segment ends with a CGI extension
        size_t dot_pos = current_path.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string extension = current_path.substr(dot_pos);

            // Check if this extension has a handler or is .cgi
            if (location->cgi_handlers.find(extension) != location->cgi_handlers.end() ||
                extension == ".cgi") {
                return true;
            }
        }

        if (next_slash == std::string::npos) {
            break;
        }
        start = next_slash + 1;
    }
    return false;
}

// Helper function to extract the script path and PATH_INFO from a CGI request
std::pair<std::string, std::string> HttpHandler::extract_cgi_components(
    const std::string& path, const LocationBlock* location) const {
    // Find the actual CGI script in the path
    std::string current_path = "";
    std::string path_info = "";
    size_t start = 0;

    while (start < path.length()) {
        size_t next_slash = path.find('/', start);
        if (next_slash == std::string::npos) {
            current_path = path;
        } else {
            current_path = path.substr(0, next_slash);
        }

        // Check if this path segment is a CGI script
        size_t dot_pos = current_path.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string extension = current_path.substr(dot_pos);

            if (location->cgi_handlers.find(extension) != location->cgi_handlers.end() ||
                extension == ".cgi") {
                // Verify the script exists
                std::string script_path = resolve_file_path(current_path, location);
                struct stat path_stat;
                if (stat(script_path.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
                    // Found the script, everything after is PATH_INFO
                    if (next_slash != std::string::npos && next_slash < path.length()) {
                        path_info = path.substr(next_slash);
                    }
                    return std::pair<std::string, std::string>(current_path, path_info);
                }
            }
        }

        if (next_slash == std::string::npos) {
            break;
        }
        start = next_slash + 1;
    }

    // No PATH_INFO, entire path is the script
    return std::pair<std::string, std::string>(path, "");
}

HttpResponse HttpHandler::handle_cgi_request(
    const HttpRequest& request, const std::string& path, const LocationBlock* location,
    const Connection* connection) {
    // Extract script path and PATH_INFO
    std::pair<std::string, std::string> cgi_components = extract_cgi_components(path, location);
    std::string script_name = cgi_components.first;
    std::string path_info = cgi_components.second;

    // Security check - validate the script path
    validate_file_access(script_name, true);  // true indicates CGI script

    // Resolve the actual file path
    std::string script_path = resolve_file_path(script_name, location);

    // Check if script exists
    struct stat path_stat;
    if (stat(script_path.c_str(), &path_stat) != 0) {
        throw HttpError(NOT_FOUND, "CGI script not found: " + script_name);
    }

    // Check if it's a regular file
    if (!S_ISREG(path_stat.st_mode)) {
        throw HttpError(FORBIDDEN, "CGI path is not a regular file");
    }

    // Check if script is executable (for .cgi files)
    size_t ext_pos = script_path.find_last_of('.');
    if (ext_pos != std::string::npos && script_path.substr(ext_pos) == ".cgi") {
        if (access(script_path.c_str(), X_OK) != 0) {
            throw HttpError(FORBIDDEN, "CGI script is not executable");
        }
    }

    // Create a modified request with PATH_INFO for the CGI executor
    HttpRequest cgi_request = request;
    cgi_request.set_path_info(path_info);
    cgi_request.set_script_name(script_name);

    // Start non-blocking CGI execution
    if (!connection) {
        throw HttpError(INTERNAL_SERVER_ERROR, "No connection context for CGI execution");
    }

    Connection* conn = const_cast<Connection*>(connection);
    if (!conn->start_cgi_execution(cgi_request, script_path, location)) {
        throw HttpError(INTERNAL_SERVER_ERROR, "Failed to start CGI execution");
    }

    // CGI started successfully, return an ACCEPTED response to indicate processing
    HttpResponse cgi_response;
    cgi_response.set_status(ACCEPTED);  // 202 status to indicate CGI is processing
    cgi_response.set_header("X-CGI-Processing", "true");  // Custom header to mark CGI processing
    return cgi_response;
}

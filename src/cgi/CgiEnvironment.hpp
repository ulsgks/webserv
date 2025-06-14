#ifndef CGI_ENVIRONMENT_HPP
#define CGI_ENVIRONMENT_HPP

#include <cctype>
#include <map>
#include <string>
#include <vector>

#include "../config/contexts/LocationBlock.hpp"
#include "../http/common/Headers.hpp"
#include "../http/common/Methods.hpp"
#include "../http/request/Request.hpp"
#include "../utils/Log.hpp"
#include "../utils/Types.hpp"

// Forward declaration to avoid circular includes
class Connection;

// CGI environment constants
static const int DEFAULT_CGI_SERVER_PORT = 8080;
static const char DEFAULT_CLIENT_IP[] = "127.0.0.1";
static const char DEFAULT_CLIENT_HOST[] = "127.0.0.1";

/**
 * Namespace for CGI environment variable handling.
 *
 * Builds CGI environment variables according to RFC 3875.
 */
namespace CgiEnvironment {

    inline CgiEnvironmentVector build(
        const HttpRequest& request, const std::string& script_path, const LocationBlock& location,
        int server_port = DEFAULT_CGI_SERVER_PORT, const std::string& client_ip = DEFAULT_CLIENT_IP,
        const std::string& client_host = DEFAULT_CLIENT_HOST);

    inline void add_request_metadata(const HttpRequest& request, CgiEnvironmentMap& env_map);

    inline void add_server_info(
        const HttpRequest& request, CgiEnvironmentMap& env_map,
        int server_port = DEFAULT_CGI_SERVER_PORT);

    inline void add_script_info(
        const HttpRequest& request, const std::string& script_path, CgiEnvironmentMap& env_map);

    inline void add_client_info(
        CgiEnvironmentMap& env_map, const std::string& client_ip = DEFAULT_CLIENT_IP,
        const std::string& client_host = DEFAULT_CLIENT_HOST);

    inline void add_http_headers(const HeaderMap& headers, CgiEnvironmentMap& env_map);

    inline CgiEnvironmentVector map_to_vector(const CgiEnvironmentMap& env_map);

    // Implementation

    inline std::vector<std::string> build(
        const HttpRequest& request, const std::string& script_path, const LocationBlock& location,
        int server_port, const std::string& client_ip, const std::string& client_host) {
        // Avoid unused parameter warning
        (void)location;

        std::map<std::string, std::string> env_map;

        // Add all CGI environment variables
        add_request_metadata(request, env_map);
        add_server_info(request, env_map, server_port);
        add_script_info(request, script_path, env_map);
        add_client_info(env_map, client_ip, client_host);
        add_http_headers(request.get_headers(), env_map);

        // Add CGI version
        env_map["GATEWAY_INTERFACE"] = "CGI/1.1";

        // Convert to vector
        return map_to_vector(env_map);
    }

    inline void add_request_metadata(
        const HttpRequest& request, std::map<std::string, std::string>& env_map) {
        // Request method
        env_map["REQUEST_METHOD"] = HttpMethods::to_string(request.get_method());

        // Request URI with query string
        env_map["REQUEST_URI"] =
            request.get_path() +
            (request.get_query_string().empty() ? "" : "?" + request.get_query_string());

        // Query string
        env_map["QUERY_STRING"] = request.get_query_string();

        // Content type and length for POST requests
        std::string content_type = request.get_header(HttpHeaders::CONTENT_TYPE);
        if (!content_type.empty()) {
            env_map["CONTENT_TYPE"] = content_type;
        }

        std::string content_length = request.get_header(HttpHeaders::CONTENT_LENGTH);
        if (!content_length.empty()) {
            env_map["CONTENT_LENGTH"] = content_length;
        }
    }

    inline void add_server_info(
        const HttpRequest& request, std::map<std::string, std::string>& env_map, int server_port) {
        // Server software
        env_map["SERVER_SOFTWARE"] = "WebServ/1.0";

        // Server name from Host header
        env_map["SERVER_NAME"] = request.get_header(HttpHeaders::HOST);

        // Server port - use provided port
        env_map["SERVER_PORT"] = Log::to_string(server_port);

        // Server protocol
        env_map["SERVER_PROTOCOL"] = request.get_http_version();
    }

    inline void add_script_info(
        const HttpRequest& request, const std::string& script_path,
        std::map<std::string, std::string>& env_map) {
        // Script name (virtual path) - use the actual script name without PATH_INFO
        env_map["SCRIPT_NAME"] =
            request.get_script_name().empty() ? request.get_path() : request.get_script_name();

        // Script filename (physical path)
        env_map["SCRIPT_FILENAME"] = script_path;

        // PATH_INFO handling
        env_map["PATH_INFO"] = request.get_path_info();

        // PATH_TRANSLATED is the physical path corresponding to PATH_INFO
        // For now, we leave it empty as it requires more complex path resolution
        env_map["PATH_TRANSLATED"] = "";
    }

    inline void add_client_info(
        std::map<std::string, std::string>& env_map, const std::string& client_ip,
        const std::string& client_host) {
        // Client information - use provided parameters
        env_map["REMOTE_ADDR"] = client_ip;
        env_map["REMOTE_HOST"] = client_host;
    }

    inline void add_http_headers(
        const HeaderMap& headers, std::map<std::string, std::string>& env_map) {
        // Group headers by name and combine multiple values with commas
        // as per RFC 3875 CGI specification
        for (HeaderMapConstIt it = headers.begin(); it != headers.end(); ++it) {
            // Convert header name to CGI environment variable format
            std::string env_name = "HTTP_";
            for (size_t i = 0; i < it->first.length(); ++i) {
                char c = it->first[i];
                if (c == '-') {
                    env_name += '_';
                } else {
                    env_name += std::toupper(c);
                }
            }

            // If environment variable already exists, append with comma separator
            // This handles multiple headers with the same name per RFC 3875
            if (env_map.find(env_name) != env_map.end()) {
                env_map[env_name] += ", " + it->second;
            } else {
                env_map[env_name] = it->second;
            }
        }
    }

    inline std::vector<std::string> map_to_vector(
        const std::map<std::string, std::string>& env_map) {
        std::vector<std::string> env_vector;

        for (CgiEnvironmentMapConstIt it = env_map.begin(); it != env_map.end(); ++it) {
            env_vector.push_back(it->first + "=" + it->second);
        }

        return env_vector;
    }

}  // namespace CgiEnvironment

#endif  // CGI_ENVIRONMENT_HPP

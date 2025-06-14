#include "../../utils/Log.hpp"
#include "../uri/Uri.hpp"
#include "Handler.hpp"

// Validate that a file path is safe to serve
void HttpHandler::validate_file_access(const std::string& path, bool cgi_script) const {
    if (is_sensitive_resource(path, cgi_script)) {
        Log::warn("Access attempt to sensitive resource: " + path);
        throw HttpError(FORBIDDEN, "Access denied to sensitive resource");
    }

    if (is_traversal_attempt(path)) {
        Log::warn("Directory traversal attempt detected: " + path);
        throw HttpError(FORBIDDEN, "Directory traversal not allowed");
    }
}

// Check for directory traversal patterns
bool HttpHandler::is_traversal_attempt(const std::string& path) const {
    if (path.find("../") != std::string::npos || path.find("..\\") != std::string::npos ||
        path == "..") {
        return true;
    }
    return false;
}

// Check if a file path is a sensitive resource that should be blocked
bool HttpHandler::is_sensitive_resource(const std::string& path, bool cgi_script) const {
    // Extract just the path part (ignore query parameters)
    std::string clean_path = Uri::extract_path(path);

    // Check for hidden files/directories (starting with dot)
    size_t last_slash = clean_path.find_last_of('/');
    if (last_slash != std::string::npos && last_slash + 1 < clean_path.length() &&
        clean_path[last_slash + 1] == '.') {
        return true;
    }

    // List of common sensitive paths to block
    const char* sensitive_patterns[] = {"/.git",      "/.svn",      "/.env",
                                        "/.htaccess", "/.htpasswd", "/.DS_Store",
                                        "/Makefile",  "/config",    "/README.md"};

    // Check against patterns
    for (size_t i = 0; i < sizeof(sensitive_patterns) / sizeof(sensitive_patterns[0]); ++i) {
        if (clean_path.find(sensitive_patterns[i]) != std::string::npos) {
            return true;
        }
    }

    // Handle file extensions based on whether this is a CGI request
    if (cgi_script) {
        // For CGI scripts, only allow specific safe extensions
        const char* allowed_cgi_extensions[] = {".cgi", ".php", ".py", ".pl", ".sh", ".rb"};

        // Extract extension
        size_t ext_pos = clean_path.find_last_of('.');
        if (ext_pos == std::string::npos) {
            return true;  // No extension, block it
        }

        std::string ext = clean_path.substr(ext_pos);

        // Check if extension is in allowed list
        for (size_t i = 0; i < sizeof(allowed_cgi_extensions) / sizeof(allowed_cgi_extensions[0]);
             ++i) {
            if (ext == allowed_cgi_extensions[i]) {
                return false;  // Allowed CGI extension
            }
        }

        return true;  // Not in allowed list, block it
    } else {
        // For regular file access, block potentially dangerous extensions
        const char* blocked_extensions[] = {
            ".conf", ".cpp", ".hpp", ".c",   ".h",  ".py", ".js", ".go", ".o",
            ".a",    ".so",  ".cgi", ".php", ".pl", ".sh", ".rb"  // Block script files for regular
                                                                  // access
        };

        for (size_t i = 0; i < sizeof(blocked_extensions) / sizeof(blocked_extensions[0]); ++i) {
            std::string ext = blocked_extensions[i];
            size_t ext_pos = clean_path.rfind(ext);

            // Check if extension is at the end of the path
            if (ext_pos != std::string::npos && ext_pos + ext.length() == clean_path.length()) {
                return true;
            }
        }
    }

    return false;
}

#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

#include <map>
#include <string>
#include <vector>

#include "../../http/common/Methods.hpp"
#include "../../utils/Types.hpp"

class LocationBlock {
   public:
    LocationBlock();

    // Configuration properties
    std::string path;                                  // URL path this location matches
    bool exact_match;                                  // Whether this is an exact path match (=)
    std::vector<HttpMethods::Method> allowed_methods;  // GET, POST, DELETE, etc.
    std::string root;                                  // Root directory for this location
    std::string index;                                 // Default file
    bool autoindex;                                    // Directory listing enabled
    std::string redirect;                              // Redirection URL if any
    int redirect_status_code;       // HTTP status code for redirect (0 if not set)
    size_t client_max_body_size;    // Upload size limit
    bool client_max_body_size_set;  // Tracking for directive inheritance
    std::string upload_store;       // Upload directory
    bool cgi_enabled;               // CGI execution allowed
    CgiHandlerMap cgi_handlers;     // Extension to CGI binary mapping
    ErrorPageMap error_pages;       // Custom error pages for this location

    std::string server_name;  // Server name for this block
    std::string listen_port;  // Listener port
    int cgi_timeout;          // Timeout for CGI

    // Validation methods - throws exceptions with descriptive error messages
    void is_valid() const;

    // Method permission checking
    bool is_allows_method(HttpMethods::Method method) const;

    // Returns a comma-separated string of allowed HTTP methods
    std::string get_allowed_methods_string() const;

   private:
    // Helper methods for validation that throw exceptions with descriptive messages
    void validate_path() const;
    void validate_methods() const;
    void validate_root() const;
    void validate_redirect_compatibilities() const;
    void validate_cgi_configuration() const;
};

#endif  // LOCATION_BLOCK_HPP

#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include "../../config/contexts/ServerBlock.hpp"
#include "../../utils/Types.hpp"
#include "../request/Request.hpp"
#include "../response/Response.hpp"

// HTTP handler constants
static const int DEFAULT_REDIRECT_STATUS = 301;  // Default redirect status (Moved Permanently)

// Forward declaration
class Connection;

class HttpHandler {
   public:
    explicit HttpHandler();
    ~HttpHandler();

    HttpResponse handle_request(
        const HttpRequest& request, const ServerBlock& server_block,
        const Connection* connection = NULL);

   private:
    // Reference to the current server block for path resolution
    const ServerBlock* server_block_;

    // Main handlers for HTTP methods
    void handle_get_request(
        const HttpRequest& request, const std::string& path, HttpResponse& response,
        const LocationBlock* location, const Connection* connection = NULL);
    void handle_post_request(
        const HttpRequest& request, HttpResponse& response, const LocationBlock* location,
        const Connection* connection = NULL);
    void handle_delete_request(
        const std::string& path, HttpResponse& response, const LocationBlock* location,
        const Connection* connection = NULL);

    // Helper methods for request validation
    void validate_post_request(const HttpRequest& request, const LocationBlock* location);
    void validate_delete_operation(const std::string& file_path);

    // Handle redirects
    void handle_redirect(
        HttpResponse& response, const std::string& redirect_url,
        int status_code = DEFAULT_REDIRECT_STATUS);

    // Content-type specific handlers for POST requests
    void handle_multipart_form_data(
        const HttpRequest& request, HttpResponse& response, const LocationBlock* location);
    void handle_urlencoded_form(
        const HttpRequest& request, HttpResponse& response, const LocationBlock* location);
    void process_multipart_upload(
        const HttpRequest& request, const std::string& boundary, HttpResponse& response,
        const LocationBlock* location);
    bool handle_file_part(
        const std::string& headers, const std::string& content, const LocationBlock* location);
    void parse_form_field(const std::string& field, FormDataMap& form_data);
    bool process_form_data(const FormDataMap& form_data, const LocationBlock* location);

    // Helper methods for GET requests
    void handle_directory_request(
        const std::string& path, const std::string& file_path, HttpResponse& response,
        const LocationBlock* location);
    void handle_file_request(const std::string& file_path, HttpResponse& response);
    bool serve_index_file(
        const std::string& dir_path, const std::string& index_name, HttpResponse& response);
    void generate_directory_listing(
        const std::string& path, const std::string& file_path, HttpResponse& response);

    // Utility function for consistent server-generated HTML styling
    std::string get_inline_css() const;

    // NON-STANDARD FEATURE: Get stylesheet reference from server configuration
    // Returns the configured default_stylesheet or empty string if none configured
    std::string get_stylesheet_link() const;

    // File path resolution helpers
    std::string resolve_file_path(
        const std::string& request_path, const LocationBlock* location = NULL) const;
    bool is_exact_match_for_location(
        const std::string& request_path, const LocationBlock* location) const;
    bool is_prefix_match_for_location(
        const std::string& request_path, const LocationBlock* location) const;
    std::string get_root_directory(const LocationBlock* location) const;
    std::string build_exact_match_path(
        const std::string& root_dir, const LocationBlock* location) const;
    std::string build_prefix_match_path(
        const std::string& root_dir, const std::string& request_path,
        const LocationBlock* location) const;

    // Security checks
    void validate_file_access(const std::string& path, bool cgi_script = false) const;
    bool is_traversal_attempt(const std::string& path) const;
    bool is_sensitive_resource(const std::string& path, bool cgi_script = false) const;

    // Error handling helpers
    HttpResponse create_error_response(
        const HttpError& e, const ServerBlock& server_block, const LocationBlock* location = NULL);
    bool try_load_error_page(
        HttpResponse& response, const std::string& resolved_path, int status_code);
    std::string resolve_error_page_path(
        const std::string& error_page_path, const LocationBlock* location);
    bool find_custom_error_page(
        HttpResponse& response, int status_code, const ServerBlock& server_block,
        const LocationBlock* location);

    // Utility functions
    std::string extract_boundary(const HttpRequest& request);
    std::string get_upload_directory(const LocationBlock* location) const;
    bool ensure_upload_directory(const std::string& dir_path) const;

    // CGI handling
    bool is_cgi_request(const std::string& path, const LocationBlock* location) const;
    HttpResponse handle_cgi_request(
        const HttpRequest& request, const std::string& path, const LocationBlock* location,
        const Connection* connection = NULL);
    CgiComponentPair extract_cgi_components(
        const std::string& path, const LocationBlock* location) const;
};

#endif  // HTTP_HANDLER_HPP

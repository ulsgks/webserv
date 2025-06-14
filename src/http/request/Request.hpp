/**
 * @file Request.hpp
 * @brief HTTP request parsing and representation.
 *
 * @see RFC 7230 - HTTP/1.1 Message Syntax and Routing
 */

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../../utils/Types.hpp"
#include "../common/Headers.hpp"
#include "../common/Methods.hpp"
#include "../error/Error.hpp"
#include "../uri/Uri.hpp"

class HttpRequest {
   public:
    // HTTP Request constants
    static const size_t MAX_URI_LENGTH = 2048;                     // Common URI length limit
    static const size_t DEFAULT_MAX_CONTENT_LENGTH = 1048576 * 8;  // 8MB default
    static const size_t MAX_HEADER_SIZE = 8192;                    // 8KB header limit
    static const size_t MAX_HEADERS = 100;                         // Maximum number of headers

    //-------------------------------------------------------------------------
    // Core functionality (Request.cpp)
    //-------------------------------------------------------------------------

    // Constructor & Destructor
    HttpRequest();
    ~HttpRequest();

    // Reset request state for reuse
    void reset();

    // Append raw HTTP request data and process it
    void append_data(const std::string& data);

    // Copy support for connection management
    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);

    //-------------------------------------------------------------------------
    // Request line parsing (Request_line.cpp)
    //-------------------------------------------------------------------------
    void parse_request_line(const std::string& line);
    void validate_method(const std::string& method_str);
    void validate_uri(const std::string& uri);
    void validate_http_version(const std::string& version);

    //-------------------------------------------------------------------------
    // Header parsing (Request_headers.cpp)
    //-------------------------------------------------------------------------
    void parse_headers(const std::string& headers_section);
    void store_header(const std::string& name, const std::string& value);
    void validate_headers();
    static std::string trim(const std::string& str);

    // Header parsing helper methods (new)
    void remove_carriage_return(std::string& line);
    bool is_header_continuation(const std::string& line);
    void store_pending_header(std::string& name, std::string& value);
    void parse_header_line(
        const std::string& line, std::string& header_name, std::string& header_value);
    bool has_whitespace_before_colon(const std::string& line, size_t colon_pos);
    bool is_malformed_header_line(const std::string& line, size_t first_colon);
    void validate_header_name(const std::string& name);
    void validate_header_value(const std::string& value);
    bool is_token_char(char c);

    //-------------------------------------------------------------------------
    // Body parsing (Request_body.cpp)
    //-------------------------------------------------------------------------
    void parse_body(const std::string& data);
    void parse_normal_body(const std::string& data);
    void parse_chunked_body(const std::string& data);

    //-------------------------------------------------------------------------
    // Accessors (Request_accessors.cpp)
    //-------------------------------------------------------------------------
    HttpMethods::Method get_method() const;
    const std::string& get_path() const;
    const std::string& get_query_string() const;
    const std::string& get_http_version() const;
    const std::string& get_body() const;
    std::string get_header(const std::string& name) const;
    const HeaderMap& get_headers() const;

    // State checks
    bool is_complete() const;
    bool is_chunked() const;
    bool is_keep_alive() const;

    // Configuration
    void set_max_content_length(size_t length);

    // CGI-specific setters and getters
    void set_path_info(const std::string& path_info) {
        path_info_ = path_info;
    }
    void set_script_name(const std::string& script_name) {
        script_name_ = script_name;
    }
    const std::string& get_path_info() const {
        return path_info_;
    }
    const std::string& get_script_name() const {
        return script_name_;
    }

    template <typename T>
    static std::string to_string(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

   private:
    // Request components
    HttpMethods::Method method_;
    Uri uri_;
    std::string http_version_;
    HeaderMap headers_;  // RFC 7230 compliant header storage
    std::string body_;

    // CGI components
    std::string path_info_;    // PATH_INFO for CGI requests
    std::string script_name_;  // SCRIPT_NAME for CGI requests

    // Parsing state
    bool headers_parsed_;
    bool complete_;
    bool chunked_;
    std::string request_buffer_;  // Buffer for incoming request data
    std::string body_buffer_;
    size_t current_chunk_size_;
    size_t max_content_length_;
    size_t header_count_;

    // Main parsing entry point
    void parse(const std::string& raw_request);

    // Path handling
    void normalize_path();
    void extract_query_string();

    // Request line parsing helpers
    void extract_request_line_components(
        const std::string& line, std::string& method_str, std::string& uri_string,
        std::string& version);
    void process_request_uri(const std::string& uri_string);
    void validate_uri_common(const std::string& uri_string);
    void process_absolute_uri(const std::string& uri_string, size_t scheme_end);

    // Chunked encoding helpers
    bool process_final_chunk();
};

#endif  // HTTP_REQUEST_HPP

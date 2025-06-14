#ifndef URI_HPP
#define URI_HPP

#include <map>
#include <sstream>
#include <string>

class Uri {
   public:
    // Constructors
    Uri();
    explicit Uri(const std::string& uri_string);

    // Parse methods
    void parse(const std::string& uri_string);

    // Absolute URI handling
    bool is_absolute() const;
    static bool is_absolute_uri(const std::string& uri_string);
    void parse_absolute_uri(const std::string& uri_string);

    // Accessor methods
    const std::string& get_path() const;
    const std::string& get_query_string() const;
    const std::string& get_scheme() const;
    const std::string& get_host() const;
    int get_port() const;

    // Query parameter methods
    std::string get_query_param(const std::string& param_name) const;
    bool has_query_param(const std::string& param_name) const;

    // Manipulation methods
    void normalize_path();

    // Utility methods
    template <typename T>
    static std::string to_string_uri(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    std::string to_string() const;
    bool is_valid() const;

    // URI validation methods
    static bool validate_uri(const std::string& uri);
    static bool validate_uri_size(const std::string& uri);
    static bool validate_uri_characters(const std::string& uri);
    static bool validate_percent_encoding(const std::string& uri, size_t index);
    static bool validate_uri_scheme(const std::string& uri);

    // Static utility methods
    static std::string encode(const std::string& str);
    static std::string decode(const std::string& str);
    static std::string decodeQueryParam(const std::string& str);
    // URI size and port constants
    static const size_t MAX_URI_LENGTH = 2048;  // RFC recommendation for URI length
    static const int HTTP_DEFAULT_PORT = 80;    // Default HTTP port
    static const int HTTPS_DEFAULT_PORT = 443;  // Default HTTPS port
    static const int MAX_PORT_NUMBER = 65535;   // Maximum valid port number

    // Extract path without query string
    static std::string extract_path(const std::string& uri);

   private:
    std::string scheme_;                               // URI scheme (e.g., "http", "https")
    std::string host_;                                 // Host component (e.g., "example.com")
    int port_;                                         // Port number (e.g., 80, 443)
    std::string path_;                                 // Path component (e.g., "/foo/bar")
    std::string query_string_;                         // Raw query string (e.g., "a=1&b=2")
    std::map<std::string, std::string> query_params_;  // Parsed query parameters
    bool absolute_;                                    // Flag indicating if URI is absolute
    bool valid_;                                       // Flag indicating if URI is valid

    void extract_query_string(const std::string& uri);
    void parse_query_params();
    void validate();
    void extract_authority_components(const std::string& authority);
};

#endif  // URI_HPP

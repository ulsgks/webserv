// HttpMethods.hpp
#ifndef HTTP_METHODS_HPP
#define HTTP_METHODS_HPP

#include <string>
#include <vector>

namespace HttpMethods {
    // Define all standard HTTP methods per RFC 7231
    // Even if we don't implement all of them, we need to recognize them
    enum Method { GET, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH, UNKNOWN };

    // Convert Method enum to string
    inline std::string to_string(Method method) {
        switch (method) {
            case GET:
                return "GET";
            case HEAD:
                return "HEAD";
            case POST:
                return "POST";
            case PUT:
                return "PUT";
            case DELETE:
                return "DELETE";
            case CONNECT:
                return "CONNECT";
            case OPTIONS:
                return "OPTIONS";
            case TRACE:
                return "TRACE";
            case PATCH:
                return "PATCH";
            default:
                return "UNKNOWN";
        }
    }

    // Convert string to Method enum
    inline Method from_string(const std::string& method_str) {
        if (method_str == "GET")
            return GET;
        if (method_str == "HEAD")
            return HEAD;
        if (method_str == "POST")
            return POST;
        if (method_str == "PUT")
            return PUT;
        if (method_str == "DELETE")
            return DELETE;
        if (method_str == "CONNECT")
            return CONNECT;
        if (method_str == "OPTIONS")
            return OPTIONS;
        if (method_str == "TRACE")
            return TRACE;
        if (method_str == "PATCH")
            return PATCH;
        return UNKNOWN;
    }

    // Check if a method string is a standard HTTP method
    inline bool is_standard_method(const std::string& method_str) {
        return method_str == "GET" || method_str == "HEAD" || method_str == "POST" ||
               method_str == "PUT" || method_str == "DELETE" || method_str == "CONNECT" ||
               method_str == "OPTIONS" || method_str == "TRACE" || method_str == "PATCH";
    }

    // Check if we actually implement/support this method in our server
    // (Different from recognizing it as a standard method)
    inline bool is_implemented(Method method) {
        return method == GET || method == POST || method == DELETE;
    }
}  // namespace HttpMethods

#endif  // HTTP_METHODS_HPP

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <map>
#include <string>

#include "../../utils/Types.hpp"
#include "../common/Headers.hpp"
#include "../common/StatusCode.hpp"

class HttpError;

class HttpResponse {
   public:
    HttpResponse();
    ~HttpResponse();

    void set_status(HttpStatusCode status);
    void set_header(const std::string& name, const std::string& value);
    void set_body(const std::string& body);
    std::string build() const;

    static HttpResponse build_default_error_response(const HttpError& error);

    // GETTERS
    std::string get_header(const std::string& name) const;
    std::string get_body() const;
    HttpStatusCode get_status() const {
        return status_;
    }

   private:
    HttpStatusCode status_;
    HeaderMap headers_;  // Changed from map to multimap
    std::string body_;

    // Helper method to set Date header
    void set_date_header();
};

#endif  // HTTP_RESPONSE_HPP

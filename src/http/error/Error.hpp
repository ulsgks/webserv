#ifndef HTTP_ERROR_HPP
#define HTTP_ERROR_HPP

#include <stdexcept>
#include <string>

#include "../common/StatusCode.hpp"

class HttpError : public std::runtime_error {
   public:
    HttpError(HttpStatusCode status, const std::string& message = "");
    virtual ~HttpError() throw();

    HttpStatusCode get_status_code() const;
    const char* get_status_message() const;
    std::string get_error_page() const;

    // New method to determine if this error should force connection closure
    bool should_close_connection() const;

   private:
    HttpStatusCode status_code_;
    void generate_error_page();
    std::string error_page_;
};

#endif  // HTTP_ERROR_HPP

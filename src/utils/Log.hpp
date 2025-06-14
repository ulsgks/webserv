#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <sstream>
#include <string>

#include "../config/contexts/LocationBlock.hpp"
#include "../config/contexts/ServerBlock.hpp"
#include "../http/request/Request.hpp"
#include "../http/response/Response.hpp"
#include "Types.hpp"

namespace Log {
    enum Level { DEBUG, INFO, WARN, ERROR, FATAL };

    // Set/get log level
    void set_level(Level level);
    Level get_level();

    // Basic logging
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message, const std::string& exception = "");
    void fatal(const std::string& message, const std::string& exception = "");

    // HTTP request/response debug overloads
    void debug(const class HttpRequest& request);
    void debug(const class HttpResponse& response);

    // ServerBlock logging overloads
    void debug(const ServerBlock& block);
    void info(const ServerBlock& block);
    void warn(const ServerBlock& block);
    void debug(const std::string& message, const ServerBlockVector& server_blocks);

    // LocationBlock logging overloads
    void debug(const LocationBlock& block);
    void info(const LocationBlock& block);
    void warn(const LocationBlock& block);

    // std::pair logging (for listen directives)
    void debug(const ListenPair& listen_pair);
    void info(const ListenPair& listen_pair);

    // Utility methods - C++98 replacement for std::to_string
    template <typename T>
    std::string to_string(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    // Internal implementation namespace
    namespace internal {
        // Log internal methods
        std::ostream& get_output_stream(Level level);
        void log(Level level, const std::string& message);
        std::string getLevelString(Level level);
        std::string getColorCode(Level level);

        // Helper methods for object formatting
        std::string format_server_block(const ServerBlock& block);
        std::string format_location_block(const LocationBlock& block);
        std::string format_listen_directive(const ListenPair& listen_pair);
    }  // namespace internal
}  // namespace Log

#endif  // LOG_HPP

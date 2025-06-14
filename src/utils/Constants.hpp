#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>

/**
 * @file Constants.hpp
 * @brief Centralized constants to eliminate magic numbers throughout the codebase
 */

namespace Constants {
    // Network and Protocol Constants
    namespace Network {
        static const int HTTP_DEFAULT_PORT = 80;
        static const int HTTPS_DEFAULT_PORT = 443;
        static const int MAX_PORT_NUMBER = 65535;
        static const int DEFAULT_SERVER_PORT = 8080;
        static const std::string DEFAULT_CLIENT_IP = "127.0.0.1";
        static const std::string DEFAULT_CLIENT_HOST = "127.0.0.1";
    }  // namespace Network

    // Buffer and Size Limits
    namespace Limits {
        static const size_t MAX_URI_LENGTH = 2048;               // 2KB URI limit
        static const size_t MAX_HEADER_SIZE = 8192;              // 8KB header limit
        static const size_t CONNECTION_BUFFER_SIZE = 32768;      // 32KB connection buffer
        static const size_t CGI_BUFFER_SIZE = 8192;              // 8KB CGI buffer
        static const size_t MAX_REQUESTS_PER_CONNECTION = 100;   // Connection request limit
        static const size_t MAX_USER_AGENT_DISPLAY_LENGTH = 50;  // User agent truncation
        static const size_t USER_AGENT_TRUNCATION_POINT = 47;    // Where to cut for "..."
        static const size_t MAX_SIZE_DIGITS = 7;                 // Max digits in size values (1GB)
        static const size_t MAX_SIZE_KB_LIMIT = 1024;            // Max KB value (1GB)
        static const size_t MAX_SIZE_MB_LIMIT = 1024;            // Max MB value (1GB)
        static const size_t MAX_SIZE_GB_LIMIT = 1;               // Max GB value
    }  // namespace Limits

    // Timeout Constants
    namespace Timeouts {
        static const time_t CONNECTION_TIMEOUT_SECONDS = 60;  // Connection timeout
        static const int CGI_TIMEOUT_SECONDS = 5;             // CGI execution timeout
        static const int POLL_TIMEOUT_MS = 1000;              // Event polling timeout
        static const int CGI_POLL_INTERVAL_USEC = 100000;     // CGI polling interval (100ms)
    }  // namespace Timeouts

    // Character and String Processing
    namespace Characters {
        static const char ASCII_PRINTABLE_MIN = 32;       // Minimum printable ASCII
        static const char ASCII_PRINTABLE_MAX = 126;      // Maximum printable ASCII
        static const int STRING_TO_NUMBER_BASE = 10;      // Base for string conversions
        static const size_t FILENAME_PREFIX_LENGTH = 10;  // Length of 'filename="'
    }  // namespace Characters

    // Size Multipliers
    namespace SizeMultipliers {
        static const size_t BYTES_PER_KB = 1024;
        static const size_t BYTES_PER_MB = 1024 * 1024;
        static const size_t BYTES_PER_GB = 1024 * 1024 * 1024;
    }  // namespace SizeMultipliers

    // HTTP Status Code Boundaries
    namespace StatusCodes {
        static const int SERVER_ERROR_THRESHOLD = 500;  // 5xx server errors start
    }
}  // namespace Constants

#endif  // CONSTANTS_HPP

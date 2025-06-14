#include "Log.hpp"

#include <iostream>
#include <sstream>

#include "../http/common/Methods.hpp"
#include "../http/common/StatusCode.hpp"

// Logging constants
static const size_t MAX_USER_AGENT_LENGTH = 50;  // Maximum length for user agent display
static const size_t USER_AGENT_TRUNCATE_LENGTH =
    47;  // Length to truncate to (leaving room for "...")

namespace {
    // Internal variables
    const char* RESET_COLOR = "\033[0m";
    Log::Level current_level_ = Log::DEBUG;  // Default log level
}  // namespace

// Implementation of internal namespace functions
namespace Log {
    // Public API implementation
    void set_level(Level level) {
        current_level_ = level;
    }

    Level get_level() {
        return current_level_;
    }

    void debug(const std::string& message) {
        if (current_level_ > DEBUG) {
            return;
        }
        internal::log(DEBUG, message);
    }

    void info(const std::string& message) {
        if (current_level_ > INFO) {
            return;
        }
        internal::log(INFO, message);
    }

    void warn(const std::string& message) {
        if (current_level_ > WARN) {
            return;
        }
        internal::log(WARN, message);
    }

    void error(const std::string& message, const std::string& exception) {
        if (current_level_ > ERROR) {
            return;
        }
        std::string fullMessage = message;
        if (!exception.empty()) {
            fullMessage += " Exception: " + exception;
        }
        internal::log(ERROR, fullMessage);
    }

    void fatal(const std::string& message, const std::string& exception) {
        if (current_level_ > FATAL) {
            return;
        }
        std::string fullMessage = message;
        if (!exception.empty()) {
            fullMessage += " Exception: " + exception;
        }
        internal::log(FATAL, fullMessage);
    }

    // HTTP request/response debug overloads
    void debug(const HttpRequest& request) {
        if (current_level_ > DEBUG) {
            return;
        }
        std::string method = HttpMethods::to_string(request.get_method());
        std::string path = request.get_path();
        std::string version = request.get_http_version();
        std::string user_agent = request.get_header("User-Agent");

        std::string message = "REQ " + method + " " + path + " " + version;
        if (!user_agent.empty()) {
            // Truncate long user agents for readability
            if (user_agent.length() > MAX_USER_AGENT_LENGTH) {
                user_agent = user_agent.substr(0, USER_AGENT_TRUNCATE_LENGTH) + "...";
            }
            message += " (" + user_agent + ")";
        }

        internal::log(DEBUG, message);
    }

    void debug(const HttpResponse& response) {
        if (current_level_ > DEBUG) {
            return;
        }
        int status_code = static_cast<int>(response.get_status());
        std::string status_message = ::get_status_message(response.get_status());
        std::string content_type = response.get_header("Content-Type");
        size_t body_size = response.get_body().size();

        std::string message = "RES " + to_string(status_code) + " " + status_message;
        if (!content_type.empty()) {
            message += " [" + content_type + "]";
        }
        message += " (" + to_string(body_size) + "B)";

        internal::log(DEBUG, message);
    }

    // ServerBlock logging implementations
    void debug(const ServerBlock& block) {
        if (current_level_ > DEBUG) {
            return;
        }
        internal::log(DEBUG, internal::format_server_block(block));
    }

    void info(const ServerBlock& block) {
        if (current_level_ > INFO) {
            return;
        }
        internal::log(INFO, internal::format_server_block(block));
    }

    void debug(const std::string& message, const std::vector<ServerBlock>& server_blocks) {
        if (current_level_ > DEBUG) {
            return;
        }
        internal::log(DEBUG, message);
        std::ostream& output = internal::get_output_stream(DEBUG);
        for (size_t i = 0; i < server_blocks.size(); ++i) {
            output << "• Server " << (i + 1) << ": "
                   << internal::format_server_block(server_blocks[i]) << std::endl;
            for (size_t j = 0; j < server_blocks[i].locations.size(); ++j) {
                output << "  - " << internal::format_location_block(server_blocks[i].locations[j])
                       << std::endl;
            }
        }
    }

    void warn(const ServerBlock& block) {
        if (current_level_ > WARN) {
            return;
        }
        internal::log(WARN, internal::format_server_block(block));
    }

    // LocationBlock logging implementations
    void debug(const LocationBlock& block) {
        if (current_level_ > DEBUG) {
            return;
        }
        internal::log(DEBUG, internal::format_location_block(block));
    }

    void info(const LocationBlock& block) {
        if (current_level_ > INFO) {
            return;
        }
        internal::log(INFO, internal::format_location_block(block));
    }

    void warn(const LocationBlock& block) {
        if (current_level_ > WARN) {
            return;
        }
        internal::log(WARN, internal::format_location_block(block));
    }

    // Listen directive logging
    void debug(const std::pair<std::string, int>& listen_pair) {
        if (current_level_ > DEBUG) {
            return;
        }
        internal::log(DEBUG, internal::format_listen_directive(listen_pair));
    }

    void info(const std::pair<std::string, int>& listen_pair) {
        if (current_level_ > INFO) {
            return;
        }
        internal::log(INFO, internal::format_listen_directive(listen_pair));
    }

    namespace internal {

        std::ostream& get_output_stream(Level level) {
            return (level >= WARN) ? std::cerr : std::cout;
        }

        void log(Level level, const std::string& message) {
            std::string levelStr = getLevelString(level);
            std::string colorCode = getColorCode(level);
            std::ostream& output = get_output_stream(level);

            output << colorCode << levelStr << RESET_COLOR << " " << message << std::endl;
        }

        std::string getLevelString(Level level) {
            switch (level) {
                case DEBUG:
                    return "[DEBUG]";
                case INFO:
                    return "[INFO] ";
                case WARN:
                    return "[WARN] ";
                case ERROR:
                    return "[ERROR]";
                case FATAL:
                    return "[FATAL]";
                default:
                    return "[UNKNOWN]";
            }
        }

        std::string getColorCode(Level level) {
            switch (level) {
                case DEBUG:
                    return "\033[30m";  // Black
                case INFO:
                    return "\033[34m";  // Blue
                case WARN:
                    return "\033[33m";  // Orange (Yellow)
                case ERROR:
                    return "\033[31m";  // Red
                case FATAL:
                    return "\033[35m";  // Pink (Magenta)
                default:
                    return "";
            }
        }

        // Helper methods for formatting
        std::string format_server_block(const ServerBlock& block) {
            std::stringstream ss;

            // Format server names more concisely
            if (!block.server_names.empty()) {
                if (block.server_names.size() == 1) {
                    ss << block.server_names[0];
                } else {
                    ss << block.server_names[0] << " +" << (block.server_names.size() - 1)
                       << " aliases";
                }
            } else {
                ss << "unnamed";
            }

            // Add default server indicator if needed
            if (block.is_default) {
                ss << " (default)";
            }

            return ss.str();
        }

        std::string format_location_block(const LocationBlock& block) {
            std::stringstream ss;
            ss << block.path << " → " << block.root;

            // Show redirect if configured
            if (!block.redirect.empty()) {
                ss << " [redirect: " << block.redirect;
                if (block.redirect_status_code > 0) {
                    ss << " (" << block.redirect_status_code << ")";
                }
                ss << "]";
            }

            // Only show methods if they're restricted
            if (block.allowed_methods.size() <
                3) {  // Assuming there are typically 3 standard methods
                ss << " (";
                for (size_t i = 0; i < block.allowed_methods.size(); ++i) {
                    if (i > 0)
                        ss << ", ";
                    ss << block.allowed_methods[i];
                }
                ss << ")";
            }

            return ss.str();
        }

        std::string format_listen_directive(const std::pair<std::string, int>& listen_pair) {
            std::stringstream ss;
            ss << "Listen [" << listen_pair.first << ":" << listen_pair.second << "]";
            return ss.str();
        }

    }  // namespace internal
}  // namespace Log

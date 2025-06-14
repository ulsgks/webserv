// src/config/parser/ParserListenDirective.cpp
#include "ConfigParser.hpp"

// Port configuration constants
static const int DEFAULT_HTTP_PORT = 80;   // Default HTTP port
static const int MIN_PORT_NUMBER = 1;      // Minimum valid port number
static const int MAX_PORT_NUMBER = 65535;  // Maximum valid port number

// Main listen directive handler
void ConfigParser::parse_listen_directive(
    ServerBlock& server, const std::vector<std::string>& values,
    const ConfigToken& directive_token) {
    // Clear any default listen directives when explicitly setting listen
    server.listen.clear();

    // Check for empty values
    if (values.empty()) {
        syntax_error("listen directive requires at least one value", directive_token);
    }

    for (size_t i = 0; i < values.size(); ++i) {
        std::pair<std::string, int> host_port = parse_host_port(values[i], directive_token);
        server.listen.push_back(host_port);
    }
}

// Parse a host:port value or standalone host/port
std::pair<std::string, int> ConfigParser::parse_host_port(
    const std::string& value, const ConfigToken& token) {
    std::string host = "0.0.0.0";  // Default host
    int port = DEFAULT_HTTP_PORT;  // Default port

    // Check if format is host:port
    size_t colon_pos = value.find(':');
    if (colon_pos != std::string::npos) {
        // Split into host and port parts
        host = value.substr(0, colon_pos);
        std::string port_str = value.substr(colon_pos + 1);

        // Validate port
        if (!is_valid_port_number(port_str, port)) {
            syntax_error("Invalid port number: " + port_str, token);
        }
    } else {
        // No colon - single value could be either host or port

        // Try as port first (unless clearly a hostname)
        if (is_valid_hostname(value)) {
            host = value;
        } else {
            // Try as port number
            if (is_valid_port_number(value, port)) {
                // Valid port, use default host
            } else {
                // Not a valid hostname or port number
                syntax_error("Invalid port number: " + value, token);
            }
        }
    }

    return std::pair<std::string, int>(host, port);
}

// Check if a string is a valid port number and extract its value
bool ConfigParser::is_valid_port_number(const std::string& str, int& port) const {
    // Must contain only digits
    for (size_t i = 0; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }

    // Convert to integer
    port = std::atoi(str.c_str());

    // Check port range
    return (port >= MIN_PORT_NUMBER && port <= MAX_PORT_NUMBER);
}

// Check if a string is a valid hostname format
bool ConfigParser::is_valid_hostname(const std::string& str) const {
    // Special cases
    if (str == "localhost" || str == "*") {
        return true;
    }

    // Hostname should have at least one dot for domain parts
    if (str.find('.') == std::string::npos) {
        return false;
    }

    // Check for valid hostname characters
    for (size_t i = 0; i < str.length(); ++i) {
        if (!std::isalnum(str[i]) && str[i] != '-' && str[i] != '.') {
            return false;
        }
    }

    return true;
}

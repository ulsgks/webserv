// src/config/parser/ParserDirectives.cpp
#include <algorithm>

#include "ConfigParser.hpp"

// Constants for directive parsing
static const size_t MAX_SIZE_DIGITS = 7;         // Maximum digits for size values (up to 1GB)
static const size_t KILOBYTE_MULTIPLIER = 1024;  // Bytes per kilobyte
static const size_t MEGABYTE_MULTIPLIER = 1024 * 1024;         // Bytes per megabyte
static const size_t GIGABYTE_MULTIPLIER = 1024 * 1024 * 1024;  // Bytes per gigabyte
static const size_t MAX_SIZE_VALUE_KB = 1024;                  // Maximum KB value (1GB)
static const size_t MAX_SIZE_VALUE_MB = 1024;                  // Maximum MB value (1GB)
static const size_t MAX_SIZE_VALUE_GB = 1;                     // Maximum GB value (1GB)

// HTTP redirect status codes
static const int REDIRECT_MOVED_PERMANENTLY = 301;
static const int REDIRECT_FOUND = 302;
static const int REDIRECT_SEE_OTHER = 303;
static const int REDIRECT_TEMPORARY_REDIRECT = 307;
static const int REDIRECT_PERMANENT_REDIRECT = 308;

void ConfigParser::parse_directive(ServerBlock& server, LocationBlock* location) {
    // Get directive name token
    ConfigToken directive = consume_token_with_check("Expected directive");
    if (directive.type != ConfigToken::IDENTIFIER) {
        syntax_error("Expected directive name", directive);
    }

    std::string name = directive.value;
    std::vector<std::string> values;

    // Parse directive values until semicolon
    while (!check_token(ConfigToken::SEMICOLON)) {
        ConfigToken value = consume_token_with_check("Unexpected end of directive");
        if (value.type != ConfigToken::IDENTIFIER && value.type != ConfigToken::STRING &&
            value.type != ConfigToken::NUMBER) {
            syntax_error("Expected directive value", value);
        }
        values.push_back(value.value);
    }

    // Consume the semicolon
    expect_token_with_error(ConfigToken::SEMICOLON, "Expected ';' after directive");

    // Process directive based on context and name
    process_directive(name, values, directive, server, location);
}

void ConfigParser::process_directive(
    const std::string& name, const std::vector<std::string>& values,
    const ConfigToken& directive_token, ServerBlock& server, LocationBlock* location) {
    // Handle server-level directives
    if (location == NULL) {
        if (name == "listen") {
            parse_listen_directive(server, values, directive_token);
        } else if (name == "server_name") {
            parse_server_name_directive(server, values, directive_token);
        } else if (name == "client_max_body_size") {
            parse_client_max_body_size_directive(
                server.client_max_body_size, server.client_max_body_size_set, values,
                directive_token);
        } else if (name == "error_page") {
            parse_error_page_server_directive(server, values, directive_token);
        } else if (name == "root") {
            // Handle server-level root directive
            expect_single_value(values, "root", directive_token);
            server.root = values[0];
        } else if (name == "default_stylesheet") {
            // NON-STANDARD FEATURE: Custom stylesheet for server-generated HTML content
            expect_single_value(values, "default_stylesheet", directive_token);
            server.default_stylesheet = values[0];
        } else if (name == "default_server" || name == "default") {
            server.is_default = true;
        } else {
            syntax_error("Unknown server directive: " + name, directive_token);
        }
    }
    // Handle location-level directives
    else {
        if (name == "methods" || name == "limit_except") {
            parse_methods_directive(*location, values, directive_token);
        } else if (name == "root") {
            expect_single_value(values, "root", directive_token);
            location->root = values[0];
        } else if (name == "index") {
            expect_single_value(values, "index", directive_token);
            location->index = values[0];
        } else if (name == "autoindex") {
            expect_single_value(values, "autoindex", directive_token);
            std::string value = values[0];
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            location->autoindex = (value == "on" || value == "true" || value == "1");
        } else if (name == "return" || name == "redirect") {
            if (values.size() != 1 && values.size() != 2) {
                syntax_error("return/redirect requires one or two values", directive_token);
            }

            // Handle redirect with status code
            if (values.size() == 2) {
                // Parse and validate the status code
                std::string status_str = values[0];
                int status_code = 0;

                // Parse status code from string
                for (size_t i = 0; i < status_str.length(); i++) {
                    if (!std::isdigit(status_str[i])) {
                        syntax_error("Invalid status code: " + status_str, directive_token);
                    }
                    status_code = status_code * 10 + (status_str[i] - '0');
                }

                // Validate that it's a redirect status code
                if (status_code != REDIRECT_MOVED_PERMANENTLY && status_code != REDIRECT_FOUND &&
                    status_code != REDIRECT_SEE_OTHER &&
                    status_code != REDIRECT_TEMPORARY_REDIRECT &&
                    status_code != REDIRECT_PERMANENT_REDIRECT) {
                    syntax_error(
                        "Invalid redirect status code: " + status_str +
                            " (must be 301, 302, 303, 307, or 308)",
                        directive_token);
                }

                location->redirect_status_code = status_code;
                location->redirect = values[1];
            } else {
                // Single value - URL only, default to 302 (temporary redirect)
                location->redirect_status_code = REDIRECT_FOUND;
                location->redirect = values[0];
            }
        } else if (name == "client_max_body_size") {
            parse_client_max_body_size_directive(
                location->client_max_body_size, location->client_max_body_size_set, values,
                directive_token);
        } else if (name == "upload_store") {
            expect_single_value(values, "upload_store", directive_token);
            location->upload_store = values[0];
        } else if (name == "error_page") {
            // Location-level error_page directive handling
            parse_error_page_location_directive(*location, values, directive_token);
        } else if (name == "cgi_handler") {
            if (values.size() != 2 || values[0].empty() || values[1].empty()) {
                syntax_error(
                    "cgi_handler requires exactly two values: extension and handler",
                    directive_token);
            }

            // Enable CGI processing
            location->cgi_enabled = true;

            // Add handler for the extension
            std::string ext = values[0];
            if (ext[0] != '.') {
                syntax_error("Extension must start with a dot (.)", directive_token);
            }
            location->cgi_handlers[ext] = values[1];
        } else {
            syntax_error("Unknown location directive: " + name, directive_token);
        }
    }
}

void ConfigParser::expect_single_value(
    const std::vector<std::string>& values, const std::string& directive_name,
    const ConfigToken& directive_token) {
    if (values.size() != 1) {
        syntax_error(directive_name + " requires exactly one value", directive_token);
    }
    if (values[0].empty()) {
        syntax_error(directive_name + " value cannot be empty", directive_token);
    }
}

void ConfigParser::parse_server_name_directive(
    ServerBlock& server, const std::vector<std::string>& values,
    const ConfigToken& directive_token) {
    // Check if values is empty or contains only empty strings
    if (values.empty()) {
        syntax_error("server_name directive requires at least one value", directive_token);
    }

    // Check if any of the server names are empty strings
    for (StringVectorConstIt it = values.begin(); it != values.end(); ++it) {
        if (it->empty()) {
            syntax_error("server_name values cannot be empty", directive_token);
        }
    }

    server.server_names = values;
}

void ConfigParser::parse_client_max_body_size_directive(
    size_t& max_size, bool& max_size_set, const std::vector<std::string>& values,
    const ConfigToken& directive_token) {
    expect_single_value(values, "client_max_body_size", directive_token);

    std::string size_str = values[0];

    // Limit to MAX_SIZE_DIGITS digits (up to 1GB in bytes)
    size_t digit_count = 0;
    size_t i = 0;
    while (i < size_str.length() && std::isdigit(size_str[i])) {
        digit_count++;
        i++;
    }
    if (digit_count > MAX_SIZE_DIGITS) {
        syntax_error("client_max_body_size value too large (max 1GB allowed)", directive_token);
    }

    // Parse the numeric part
    size_t size_value = 0;
    for (i = 0; i < digit_count; i++) {
        size_value = size_value * 10 + (size_str[i] - '0');
    }

    // Process the unit if present
    if (i < size_str.length()) {
        char unit = std::tolower(size_str[i]);

        // Maximum reasonable values for each unit for a school project
        if (unit == 'k' && size_value > MAX_SIZE_VALUE_KB) {  // Max 1GB in KB
            syntax_error(
                "client_max_body_size exceeds maximum allowed size (1GB)", directive_token);
        } else if (unit == 'm' && size_value > MAX_SIZE_VALUE_MB) {  // Max 1GB in MB
            syntax_error(
                "client_max_body_size exceeds maximum allowed size (1GB)", directive_token);
        } else if (unit == 'g' && size_value > MAX_SIZE_VALUE_GB) {  // Max 1GB in GB
            syntax_error(
                "client_max_body_size exceeds maximum allowed size (1GB)", directive_token);
        } else if (unit != 'k' && unit != 'm' && unit != 'g') {
            syntax_error("Invalid size unit: " + std::string(1, unit), directive_token);
        }

        // Apply the multiplier
        if (unit == 'k')
            size_value *= KILOBYTE_MULTIPLIER;
        else if (unit == 'm')
            size_value *= MEGABYTE_MULTIPLIER;
        else if (unit == 'g')
            size_value *= GIGABYTE_MULTIPLIER;
    }

    max_size = size_value;
    max_size_set = true;
}

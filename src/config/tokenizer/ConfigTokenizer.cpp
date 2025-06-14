// src/config/configTokenizer.cpp
#include "ConfigTokenizer.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "../../utils/Log.hpp"

// ConfigToken implementation
ConfigToken::ConfigToken(Type type, const std::string& value, int line, int col)
    : type(type), value(value), line(line), column(col) {
}

// ConfigTokenizer implementation
ConfigTokenizer::ConfigTokenizer() : current_type_(ConfigToken::IDENTIFIER), line_(1), column_(0) {
}

std::vector<ConfigToken> ConfigTokenizer::tokenize(const std::string& filename) {
    filename_ = filename;
    tokens_.clear();
    current_token_.clear();
    line_ = 1;
    column_ = 0;

    file_.open(filename.c_str());
    if (!file_.is_open()) {
        syntax_error("Failed to open configuration file");
    }

    char c;
    while (file_.get(c)) {
        column_++;

        if (c == '\n') {
            line_++;
            column_ = 0;
            continue;
        }

        if (c == '#') {
            // Skip comments
            while (file_.get(c) && c != '\n') {
            }
            line_++;
            column_ = 0;
            continue;
        }

        if (c == '"') {
            // Handle quoted strings
            process_string();
            continue;
        }

        process_character(c);
    }

    // Handle any remaining token
    if (!current_token_.empty()) {
        add_token(current_type_, current_token_);
        current_token_.clear();
    }

    // Add end of file token
    add_token(ConfigToken::END_OF_FILE, "");

    file_.close();
    return tokens_;
}

void ConfigTokenizer::process_character(char c) {
    if (current_type_ == ConfigToken::IDENTIFIER) {
        if (current_token_.empty()) {
            if (is_whitespace(c)) {
                return;
            } else if (c == '{') {
                add_token(ConfigToken::OPEN_BRACE, "{");
            } else if (c == '}') {
                add_token(ConfigToken::CLOSE_BRACE, "}");
            } else if (c == ';') {
                add_token(ConfigToken::SEMICOLON, ";");
            } else if (c == '=') {
                // Handle equals sign as a separate token
                add_token(ConfigToken::EQUALS, "=");
            } else if (is_identifier_start(c)) {
                current_token_ += c;
            } else if (std::isdigit(c)) {
                current_token_ += c;
                current_type_ = ConfigToken::NUMBER;
            } else {
                syntax_error(std::string("Unexpected character '") + c + "'");
            }
        } else {
            if (is_identifier_part(c)) {
                current_token_ += c;
                // Check length after adding the character
                if (current_token_.length() > MAX_TOKEN_LENGTH) {
                    syntax_error("Identifier token exceeds maximum allowed length");
                }
            } else if (is_whitespace(c) || c == '{' || c == '}' || c == ';' || c == '=') {
                add_token(ConfigToken::IDENTIFIER, current_token_);
                current_token_.clear();
                process_character(c);
            } else {
                syntax_error(std::string("Unexpected character '") + c + "' in identifier");
            }
        }
    } else if (current_type_ == ConfigToken::NUMBER) {
        if (std::isdigit(c)) {
            current_token_ += c;
            // Check length after adding the digit
            if (current_token_.length() > MAX_TOKEN_LENGTH) {
                syntax_error("Number token exceeds maximum allowed length");
            }
        } else if (c == 'k' || c == 'K' || c == 'm' || c == 'M' || c == 'g' || c == 'G') {
            // Size units for client_max_body_size
            current_token_ += c;
            if (current_token_.length() > MAX_TOKEN_LENGTH) {
                syntax_error("Number token exceeds maximum allowed length");
            }
            add_token(ConfigToken::NUMBER, current_token_);
            current_token_.clear();
            current_type_ = ConfigToken::IDENTIFIER;
        } else if (c == '.') {  // Add this section to allow dots for IP addresses
            current_token_ += c;
            if (current_token_.length() > MAX_TOKEN_LENGTH) {
                syntax_error("Number token exceeds maximum allowed length");
            }
        } else if (c == ':') {  // Also handle colons for host:port format
            current_token_ += c;
            if (current_token_.length() > MAX_TOKEN_LENGTH) {
                syntax_error("Number token exceeds maximum allowed length");
            }
        } else if (is_whitespace(c) || c == ';' || c == '=') {
            add_token(ConfigToken::NUMBER, current_token_);
            current_token_.clear();
            current_type_ = ConfigToken::IDENTIFIER;
            process_character(c);
        } else {
            syntax_error(std::string("Unexpected character '") + c + "' in number");
        }
    }
}

void ConfigTokenizer::process_string() {
    std::string str;
    char c;
    bool escaped = false;

    while (file_.get(c)) {
        column_++;

        if (c == '\n') {
            line_++;
            column_ = 0;
            // Unterminated string literal
            syntax_error("Unterminated string literal");
        }

        if (escaped) {
            str += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            // End of string
            if (str.length() > MAX_TOKEN_LENGTH) {
                syntax_error("String token exceeds maximum allowed length");
            }
            add_token(ConfigToken::STRING, str);
            return;
        } else {
            str += c;
            // Check length frequently to avoid memory issues with huge strings
            if (str.length() > MAX_TOKEN_LENGTH) {
                syntax_error("String token exceeds maximum allowed length");
            }
        }
    }

    // If we get here, the file ended before the string was closed
    syntax_error("Unterminated string literal at end of file");
}

void ConfigTokenizer::add_token(ConfigToken::Type type, const std::string& value) {
    tokens_.push_back(ConfigToken(type, value, line_, column_ - value.length()));
}

bool ConfigTokenizer::is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool ConfigTokenizer::is_identifier_start(char c) {
    // Allow forward slash and dots as valid starting characters for paths and extensions
    // Also allow letters for scheme-based URLs (http, https, etc.)
    return std::isalpha(c) || c == '_' || c == '/' || c == '.';
}

bool ConfigTokenizer::is_identifier_part(char c) {
    // Allow standard identifier characters plus URL characters
    // This includes query parameters (?key=value&key2=value2) and schemes (http://, https:// )
    return std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/' || c == ':' ||
           c == '?' || c == '&' || c == '=' || c == '#' || c == '%';
}

void ConfigTokenizer::syntax_error(const std::string& message) {
    std::ostringstream error;

    error << "Syntax error: " << filename_;
    if (line_ != -1) {
        // Format for clickable paths in IDEs
        error << ":" << line_ << ":" << column_;
    }
    error << ": " << message;

    throw std::runtime_error(error.str());
}

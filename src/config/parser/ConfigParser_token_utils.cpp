// src/config/parser/ParserTokenHelpers.cpp
#include "ConfigParser.hpp"

bool ConfigParser::check_token(ConfigToken::Type type, const std::string& expected_value) {
    if (current_token_ >= tokens_.size()) {
        return false;
    }

    ConfigToken token = tokens_[current_token_];

    if (token.type != type) {
        return false;
    }

    if (!expected_value.empty() && token.value != expected_value) {
        return false;
    }

    return true;
}

bool ConfigParser::match_token(ConfigToken::Type type, const std::string& expected_value) {
    if (check_token(type, expected_value)) {
        current_token_++;
        return true;
    }
    return false;
}

void ConfigParser::expect_token_with_error(
    ConfigToken::Type type, const std::string& error_message) {
    if (!match_token(type)) {
        syntax_error(error_message, get_current_token());
    }
}

ConfigToken ConfigParser::consume_token_with_check(const std::string& error_message) {
    if (current_token_ >= tokens_.size()) {
        // Use the last token if we're at the end
        syntax_error(error_message, tokens_.back());
    }

    return tokens_[current_token_++];
}

ConfigToken ConfigParser::get_current_token() {
    if (current_token_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_token_];
}

ConfigToken ConfigParser::get_last_token() {
    if (current_token_ > 0 && current_token_ <= tokens_.size()) {
        return tokens_[current_token_ - 1];
    }
    return tokens_.back();
}

ConfigToken ConfigParser::peek_token() {
    if (current_token_ >= tokens_.size()) {
        return ConfigToken(ConfigToken::END_OF_FILE, "", -1, -1);
    }

    return tokens_[current_token_];
}

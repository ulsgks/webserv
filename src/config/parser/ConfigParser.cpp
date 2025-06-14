// src/config/parser/ParserCore.cpp
#include "ConfigParser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include "../../utils/Log.hpp"

ConfigParser::ConfigParser() : current_token_(0) {
}

std::vector<ServerBlock> ConfigParser::parse(const std::string& filename) {
    ConfigTokenizer tokenizer;
    tokens_ = tokenizer.tokenize(filename);
    current_token_ = 0;
    server_blocks_.clear();
    current_filename_ = filename;  // Store the filename for error messages

    // check if the file is empty
    if (tokens_.size() <= 1) {
        throw std::runtime_error("Empty configuration file");
    }

    while (current_token_ < tokens_.size() &&
           tokens_[current_token_].type != ConfigToken::END_OF_FILE) {
        if (match_token(ConfigToken::IDENTIFIER, "server")) {
            parse_server_block();
        } else {
            syntax_error("Expected 'server' block", get_current_token());
        }
    }

    return server_blocks_;
}

void ConfigParser::syntax_error(const std::string& message, const ConfigToken& token) {
    std::string filename = current_filename_;

    std::ostringstream error;

    error << "Syntax error: " << filename;
    if (token.line != -1) {
        // Format for clickable paths in IDEs
        error << ":" << token.line << ":" << token.column;
    }
    error << ": " << message;

    throw std::runtime_error(error.str());
}

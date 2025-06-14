// src/config/parser/ParserServerBlock.cpp
#include "../../utils/Log.hpp"
#include "ConfigParser.hpp"

void ConfigParser::parse_server_block() {
    ServerBlock server;

    expect_token_with_error(ConfigToken::OPEN_BRACE, "Expected '{' after 'server'");

    while (!check_token(ConfigToken::CLOSE_BRACE)) {
        if (match_token(ConfigToken::IDENTIFIER, "location")) {
            parse_location_block(server);
        } else {
            parse_directive(server);
        }
    }

    expect_token_with_error(ConfigToken::CLOSE_BRACE, "Expected '}' to close server block");

    try {
        server.is_valid();
    } catch (const std::exception& e) {
        syntax_error("Invalid server block: " + std::string(e.what()), get_last_token());
    }

    server_blocks_.push_back(server);
}

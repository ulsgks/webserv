// src/config/parser/ParserLocationBlock.cpp
#include "ConfigParser.hpp"

// src/config/parser/location_block.cpp
void ConfigParser::parse_location_block(ServerBlock& server) {
    LocationBlock location;

    // Check for the equals sign for exact matching
    if (check_token(ConfigToken::EQUALS)) {
        // Consume the equals token
        match_token(ConfigToken::EQUALS);
        location.exact_match = true;
    }

    // Parse location path
    ConfigToken path_token = consume_token_with_check("Expected location path");
    if (path_token.type != ConfigToken::IDENTIFIER && path_token.type != ConfigToken::STRING) {
        syntax_error("Expected location path", path_token);
    }
    location.path = path_token.value;

    // Ensure path starts with / - CHANGED: Reporting error instead of fixing
    if (location.path.empty() || location.path[0] != '/') {
        syntax_error("Location path must start with a slash (/)", path_token);
    }

    expect_token_with_error(ConfigToken::OPEN_BRACE, "Expected '{' after location path");

    while (!check_token(ConfigToken::CLOSE_BRACE)) {
        parse_directive(server, &location);
    }

    expect_token_with_error(ConfigToken::CLOSE_BRACE, "Expected '}' to close location block");

    try {
        location.is_valid();
    } catch (const std::exception& e) {
        syntax_error(
            "Invalid location block for '" + location.path + "': " + std::string(e.what()),
            get_last_token());
    }

    server.locations.push_back(location);
}

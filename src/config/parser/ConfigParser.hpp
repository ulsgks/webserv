// src/config/configParser.hpp
#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

#include "../../utils/Types.hpp"
#include "../contexts/ServerBlock.hpp"
#include "../tokenizer/ConfigTokenizer.hpp"

// Parses configuration files into ServerBlock objects
class ConfigParser {
   public:
    ConfigParser();

    // Parse a configuration file into server blocks
    ServerBlockVector parse(const std::string& filename);

   private:
    // Parsing methods
    void parse_server_block();
    void parse_location_block(ServerBlock& server);
    void parse_directive(ServerBlock& server, LocationBlock* location = NULL);

    // Process a directive based on its name and context
    void process_directive(
        const std::string& name, const DirectiveValues& values, const ConfigToken& directive_token,
        ServerBlock& server, LocationBlock* location = NULL);

    // Helper methods for token handling
    bool check_token(ConfigToken::Type type, const std::string& expected_value = "");
    bool match_token(ConfigToken::Type type, const std::string& expected_value = "");
    void expect_token_with_error(ConfigToken::Type type, const std::string& error_message);
    ConfigToken consume_token_with_check(const std::string& error_message);
    ConfigToken get_current_token();
    ConfigToken get_last_token();
    ConfigToken peek_token();

    // Directive parsing helpers
    void parse_listen_directive(
        ServerBlock& server, const DirectiveValues& values, const ConfigToken& directive_token);
    ListenPair parse_host_port(const std::string& value, const ConfigToken& token);
    bool is_valid_port_number(const std::string& str, int& port) const;
    bool is_valid_hostname(const std::string& str) const;

    void parse_server_name_directive(
        ServerBlock& server, const DirectiveValues& values, const ConfigToken& directive_token);
    void parse_client_max_body_size_directive(
        size_t& max_size, bool& max_size_set, const DirectiveValues& values,
        const ConfigToken& directive_token);
    void parse_error_page_server_directive(
        ServerBlock& server, const DirectiveValues& values, const ConfigToken& directive_token);
    void parse_error_page_location_directive(
        LocationBlock& location, const DirectiveValues& values, const ConfigToken& directive_token);
    void parse_error_page(
        std::map<int, std::string>& error_pages, const DirectiveValues& values,
        const ConfigToken& directive_token);
    void parse_methods_directive(
        LocationBlock& location, const DirectiveValues& values, const ConfigToken& directive_token);

    // Validation helpers
    void expect_single_value(
        const DirectiveValues& values, const std::string& directive_name,
        const ConfigToken& directive_token);

    // Error reporting
    void syntax_error(const std::string& message, const ConfigToken& token);

    // State variables
    std::vector<ConfigToken> tokens_;  // Tokens from the tokenizer
    size_t current_token_;             // Index of current token
    ServerBlockVector server_blocks_;  // Output server blocks
    std::string current_filename_;     // Current file being parsed
};

#endif  // CONFIG_PARSER_HPP

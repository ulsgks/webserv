// src/config/tokenizer/ConfigTokenizer.hpp
#ifndef CONFIG_TOKENIZER_HPP
#define CONFIG_TOKENIZER_HPP

#include <fstream>
#include <string>
#include <vector>

// Represents a token in the configuration file
class ConfigToken {
   public:
    // Token types
    enum Type {
        IDENTIFIER,   // Directive names and values
        OPEN_BRACE,   // {
        CLOSE_BRACE,  // }
        SEMICOLON,    // ;
        EQUALS,       // = (for exact location matching)
        STRING,       // "quoted string"
        NUMBER,       // numeric value
        END_OF_FILE   // End of file marker
    };

    // Constructor
    ConfigToken(Type type, const std::string& value, int line, int col);

    // Token properties
    Type type;          // Type of token
    std::string value;  // Token value
    int line;           // Line number in the source file
    int column;         // Column number in the source file
};

// Tokenizes configuration files
class ConfigTokenizer {
   public:
    ConfigTokenizer();

    // Tokenize a configuration file into a vector of tokens
    std::vector<ConfigToken> tokenize(const std::string& filename);

   private:
    // Constants
    static const size_t MAX_TOKEN_LENGTH = 4096;  // 4KB maximum token length

    // Helper methods for tokenization
    void process_character(char c);
    void add_token(ConfigToken::Type type, const std::string& value);
    bool is_whitespace(char c);
    bool is_identifier_start(char c);
    bool is_identifier_part(char c);
    void process_string();

    // Error reporting
    void syntax_error(const std::string& message);

    // State variables
    std::string current_token_;        // Current token being built
    ConfigToken::Type current_type_;   // Type of current token
    std::string filename_;             // Name of file being tokenized
    int line_;                         // Current line number
    int column_;                       // Current column number
    std::vector<ConfigToken> tokens_;  // Output tokens
    std::ifstream file_;               // Input file stream
};

#endif  // CONFIG_TOKENIZER_HPP

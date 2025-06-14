#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../utils/Types.hpp"
#include "LocationBlock.hpp"

class ServerBlock {
   public:
    ServerBlock();

    // Server identification
    StringVector server_names;
    ListenVector listen;

    std::string root;
    bool client_max_body_size_set;
    size_t client_max_body_size;
    ErrorPageMap error_pages;

    // NON-STANDARD FEATURE: Custom stylesheet for server-generated HTML content
    // This allows configuring a CSS file to style directory listings, error pages, etc.
    std::string default_stylesheet;

    LocationBlockVector locations;

    bool is_default;

    // Validation methods - throws exceptions with descriptive error messages
    void is_valid() const;

    // Utility methods
    const LocationBlock* match_location(const std::string& uri) const;
    bool matches_server_name(const std::string& host) const;
    std::string normalize_server_name(const std::string& name) const;

   private:
    // Helper methods for validation that throw exceptions with descriptive messages
    void validate_listen_directives() const;
    void validate_locations() const;
    void validate_root() const;
};

#endif  // SERVER_BLOCK_HPP

// src/config/Config.hpp
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <string>
#include <vector>

#include "../config/contexts/ServerBlock.hpp"
#include "../utils/Types.hpp"

namespace Config {
    // Main interface methods - public API
    void load_config(const std::string& filename, ServerBlockVector& server_blocks);

    // Internal implementation namespace - not meant for public use
    namespace internal {
        // Configuration validation methods
        void validate_server_blocks(const ServerBlockVector& blocks);
        void check_duplicate_server_names(const ServerBlockVector& blocks);
        void check_duplicate_default_servers(const ServerBlockVector& blocks);
        void validate_location_roots(const ServerBlock& server);

        // Directive inheritance
        void inherit_client_max_body_size(ServerBlockVector& server_blocks);

        // Fallback configuration
        void load_default_config(ServerBlockVector& server_blocks);
    }  // namespace internal
}  // namespace Config

#endif  // CONFIG_HPP

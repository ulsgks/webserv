// src/config/config.cpp
#include "Config.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <utility>

#include "../config/parser/ConfigParser.hpp"
#include "../http/common/Methods.hpp"
#include "../utils/Log.hpp"

// Default configuration constants
static const int DEFAULT_EXAMPLE_PORT = 8080;  // Port for example server
static const int DEFAULT_MAIN_PORT = 4242;     // Port for main default server

// Implementation of internal namespace functions
namespace Config {

    // Public API implementation
    void load_config(const std::string& filename, std::vector<ServerBlock>& server_blocks) {
        // If no filename is specified, load default configuration
        if (filename.empty()) {
            internal::load_default_config(server_blocks);
            return;
        }

        try {
            ConfigParser parser;
            server_blocks = parser.parse(filename);
            internal::inherit_client_max_body_size(server_blocks);
            internal::validate_server_blocks(server_blocks);
        } catch (const std::exception& e) {
            Log::error(e.what());
            throw std::runtime_error("Configuration file is not valid");
        }
    }

    namespace internal {

        void inherit_client_max_body_size(std::vector<ServerBlock>& server_blocks) {
            for (ServerBlockVectorIt server = server_blocks.begin(); server != server_blocks.end();
                 ++server) {
                for (LocationBlockVectorIt location = server->locations.begin();
                     location != server->locations.end(); ++location) {
                    // Apply client_max_body_size inheritance if not explicitly set
                    if (!location->client_max_body_size_set) {
                        location->client_max_body_size = server->client_max_body_size;
                    }
                }
            }
        }

        void validate_location_roots(const ServerBlock& server) {
            // For each location without its own root, ensure server has one
            for (LocationBlockVectorConstIt loc = server.locations.begin();
                 loc != server.locations.end(); ++loc) {
                // Skip locations that don't need a root
                if (!loc->root.empty() || !loc->redirect.empty() || loc->cgi_enabled) {
                    continue;
                }

                // If location has no root, server must have one
                if (server.root.empty()) {
                    throw std::runtime_error(
                        "Location '" + loc->path +
                        "' has no root directive and server block has no root directive");
                }
            }
        }

        void check_duplicate_server_names(const std::vector<ServerBlock>& blocks) {
            // Structure to track (server_name, port) combinations
            ServerNameMap server_name_ports;

            for (ServerBlockVectorConstIt block = blocks.begin(); block != blocks.end(); ++block) {
                // Each server block can have multiple server_names and listen directives
                for (StringVectorConstIt name = block->server_names.begin();
                     name != block->server_names.end(); ++name) {
                    std::string normalized_name = block->normalize_server_name(*name);

                    // Check this server_name with each listen port
                    for (ListenVectorConstIt listen = block->listen.begin();
                         listen != block->listen.end(); ++listen) {
                        int port = listen->second;
                        ServerNamePortPair name_port_pair =
                            ServerNamePortPair(normalized_name, port);

                        // Check if this name+port combination already exists
                        ServerNameMapIt it = server_name_ports.find(name_port_pair);

                        if (it != server_name_ports.end()) {
                            std::string error_msg =
                                "Duplicate server name + port combination: " + *name + " on port " +
                                Log::to_string(port);
                            throw std::runtime_error(error_msg);
                        }

                        // Store original name for better error messages
                        server_name_ports[name_port_pair] = *name;
                    }
                }
            }
        }

        void check_duplicate_default_servers(const std::vector<ServerBlock>& blocks) {
            ListenDefaultMap default_servers;

            for (ServerBlockVectorConstIt block = blocks.begin(); block != blocks.end(); ++block) {
                for (ListenVectorConstIt listen = block->listen.begin();
                     listen != block->listen.end(); ++listen) {
                    // If this server block is marked as default
                    if (block->is_default) {
                        // Check if we already have a default server for this host:port
                        ListenDefaultMapIt it = default_servers.find(*listen);
                        if (it != default_servers.end()) {
                            std::string error_msg = "Multiple default servers for " +
                                                    listen->first + ":" +
                                                    Log::to_string(listen->second);
                            throw std::runtime_error(error_msg);
                        }

                        // Register this as the default server for this host:port
                        default_servers[*listen] = &(*block);
                    }
                }
            }
        }

        void validate_server_blocks(const std::vector<ServerBlock>& blocks) {
            if (blocks.empty()) {
                throw std::runtime_error("No server blocks defined");
            }

            // Check each server block individually
            for (ServerBlockVectorConstIt it = blocks.begin(); it != blocks.end(); ++it) {
                // Validate that locations without root have a server-level root
                internal::validate_location_roots(*it);
            }

            // Check for duplicate server names and listen directives
            internal::check_duplicate_server_names(blocks);
            internal::check_duplicate_default_servers(blocks);
        }

        void load_default_config(std::vector<ServerBlock>& server_blocks) {
            server_blocks.clear();

            // Create two example server blocks
            ServerBlock server1;
            server1.server_names.push_back("example.com");
            server1.listen.push_back(std::pair<std::string, int>("0.0.0.0", DEFAULT_EXAMPLE_PORT));
            server1.root = "www/";
            {
                LocationBlock loc;
                loc.index = "index.html";
                loc.path = "/";
                loc.allowed_methods.push_back(HttpMethods::GET);
                server1.locations.push_back(loc);
            }

            ServerBlock server2;  // Default server
            server2.is_default = true;
            server2.listen.push_back(std::pair<std::string, int>("0.0.0.0", DEFAULT_MAIN_PORT));
            server2.root = "www/";
            {
                LocationBlock loc;
                loc.index = "index.html";
                loc.path = "/";
                loc.autoindex = true;
                loc.allowed_methods.push_back(HttpMethods::GET);
                loc.allowed_methods.push_back(HttpMethods::POST);
                server2.locations.push_back(loc);
            }

            server_blocks.push_back(server1);
            server_blocks.push_back(server2);
        }

    }  // namespace internal
}  // namespace Config

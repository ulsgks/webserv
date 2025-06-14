#include <cstdlib>
#include <iostream>
#include <string>

#include "server/Server.hpp"
#include "utils/Log.hpp"
#include "utils/Options.hpp"
#include "utils/Signals.hpp"

int main(int argc, char* argv[]) {
    try {
        // Set up signal handlers first
        Signals::setup_handlers();

        // Parse command line options
        Options options = OptionsParser::parse(argc, argv);

        // Show help message if requested
        if (options.show_help) {
            OptionsParser::print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

        // Apply options (sets up logging, etc.)
        OptionsParser::apply(options);

        // Create and run the server
        Server server(options.config_file);
        server.run();
    } catch (const std::exception& e) {
        Log::fatal("webserv: " + std::string(e.what()));
        return EXIT_FAILURE;
    } catch (...) {
        Log::fatal("webserv: Unknown exception occurred");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

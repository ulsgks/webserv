#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <unistd.h>

#include <iostream>
#include <string>

#include "../utils/Log.hpp"

// Command-line options structure
struct Options {
    std::string config_file;  // Path to the configuration file
    bool show_help;           // Flag to show help message
    bool verbose_logging;     // Enable verbose logging

    // Constructor with default values
    Options() : config_file(""), show_help(false), verbose_logging(false) {
    }
};

namespace OptionsParser {
    // Display usage information
    void print_usage(const char* program_name) {
        std::cout << "Usage: " << program_name << " [options] [config_file]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -c <file>   Specify configuration file" << std::endl;
        std::cout << "  -v          Enable verbose logging" << std::endl;
        std::cout << "  -h          Display this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Config file can be specified either with -c flag or as a positional argument."
                  << std::endl;
        std::cout << "If not specified, default configuration will be used." << std::endl;
    }

    // Parse command line arguments into Options structure
    Options parse(int argc, char* argv[]) {
        Options options;
        int opt;

        // Parse command line options
        while ((opt = getopt(argc, argv, "c:vh")) != -1) {
            switch (opt) {
                case 'c':
                    options.config_file = optarg;
                    break;
                case 'v':
                    options.verbose_logging = true;
                    break;
                case 'h':
                    options.show_help = true;
                    break;
                default:
                    // Invalid option
                    options.show_help = true;
                    return options;
            }
        }

        // Check for positional arguments (config file without -c flag)
        if (optind < argc) {
            // If config file was already set with -c, this is an error
            if (!options.config_file.empty()) {
                std::cerr
                    << "Error: Config file specified both with -c flag and as positional argument"
                    << std::endl;
                options.show_help = true;
                return options;
            }

            // If more than one positional argument, this is an error
            if (optind + 1 < argc) {
                std::cerr << "Error: Too many arguments" << std::endl;
                options.show_help = true;
                return options;
            }

            // Set config file from positional argument
            options.config_file = argv[optind];
        }

        return options;
    }

    // Apply parsed options to the program
    void apply(const Options& options) {
        // Set logging level based on verbose flag
        if (options.verbose_logging) {
            Log::set_level(Log::DEBUG);
        } else {
            Log::set_level(Log::INFO);
        }
    }
}  // namespace OptionsParser

#endif  // OPTIONS_HPP

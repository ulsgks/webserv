#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <signal.h>

namespace Signals {
    // Initialize signal handlers for the server
    void setup_handlers();

    // Check if the server should continue running
    bool should_continue();

    // Signal handler function for SIGINT and SIGTERM
    void signal_handler(int signal);
}  // namespace Signals

#endif  // SIGNALS_HPP

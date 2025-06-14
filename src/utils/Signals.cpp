#include "Signals.hpp"

#include "../utils/Log.hpp"

// Global flag to indicate server running state
volatile sig_atomic_t g_running = 1;

namespace Signals {

    void setup_handlers() {
        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        // Set up handlers for SIGINT (Ctrl+C) and SIGTERM
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            Log::warn("Failed to set SIGINT handler");
        }

        if (sigaction(SIGTERM, &sa, NULL) == -1) {
            Log::warn("Failed to set SIGTERM handler");
        }

        // Ignore SIGPIPE to prevent server from crashing when a client disconnects unexpectedly
        signal(SIGPIPE, SIG_IGN);
    }

    bool should_continue() {
        return g_running != 0;
    }

    void signal_handler(int signal) {
        if (signal == SIGINT) {
            Log::warn("Received SIGINT (Ctrl+C), initiating graceful shutdown");
            g_running = 0;
        } else if (signal == SIGTERM) {
            Log::info("Received SIGTERM, initiating graceful shutdown");
            g_running = 0;
        }
    }

}  // namespace Signals

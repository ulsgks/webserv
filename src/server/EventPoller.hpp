#ifndef EVENTPOLLER_HPP
#define EVENTPOLLER_HPP

#include <poll.h>

#include <map>
#include <vector>

#include "../utils/Types.hpp"

// Structure to hold poll results
struct PollEvents {
    static const short READ = POLLIN | POLLPRI;     // Data available to read
    static const short WRITE = POLLOUT;             // Buffer space available for writing
    static const short ERROR = POLLERR | POLLNVAL;  // Real errors only
    static const short HUP = POLLHUP;               // Peer closed their end
};

struct PollResult {
    int fd;          // File descriptor that had an event
    bool can_read;   // True if fd is ready for reading
    bool can_write;  // True if fd is ready for writing
    bool has_error;  // True if fd has an error condition
    bool has_hup;    // True if peer closed their end (POLLHUP)

    PollResult() : fd(-1), can_read(false), can_write(false), has_error(false), has_hup(false) {
    }
};

class EventPoller {
   private:
    std::vector<struct pollfd> poll_fds_;  // List of file descriptors to monitor
    EventMap fd_events_;                   // Maps fd to its desired events

    // Poll timeout constants
    static const int POLL_TIMEOUT_MS = 1000;  // Poll timeout in milliseconds (1 second)

   public:
    EventPoller();
    ~EventPoller();

    // File descriptor management
    void watch_fd(int fd, short events);       // Add fd to monitoring
    void update_events(int fd, short events);  // Modify events for fd
    void unwatch_fd(int fd);                   // Remove fd from monitoring

    // Poll for events and return results
    PollResultVector poll_once();  // Do one poll() and return results

   private:
    PollResult create_poll_result(const struct pollfd& pfd);
};

#endif  // EVENTPOLLER_HPP

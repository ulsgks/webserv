#include "EventPoller.hpp"

#include <cerrno>
#include <stdexcept>

EventPoller::EventPoller() {
}

EventPoller::~EventPoller() {
}

// ----------------------------------------------------------------------------
// File descriptor management
// ----------------------------------------------------------------------------

void EventPoller::watch_fd(int fd, short events) {
    // Check if fd already exists
    if (fd_events_.find(fd) != fd_events_.end()) {
        throw std::runtime_error("File descriptor already being monitored");
    }

    // Add to our tracking containers
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;

    poll_fds_.push_back(pfd);
    fd_events_[fd] = events;
}

void EventPoller::update_events(int fd, short events) {
    // Find and modify the fd's events
    for (size_t i = 0; i < poll_fds_.size(); ++i) {
        if (poll_fds_[i].fd == fd) {
            poll_fds_[i].events = events;
            fd_events_[fd] = events;
            return;
        }
    }
    throw std::runtime_error("File descriptor not found");
}

void EventPoller::unwatch_fd(int fd) {
    // Remove from vector
    for (PollFdVectorIt it = poll_fds_.begin(); it != poll_fds_.end();) {
        if (it->fd == fd) {
            it = poll_fds_.erase(it);
        } else {
            ++it;
        }
    }

    // Remove from map
    fd_events_.erase(fd);
}

// ----------------------------------------------------------------------------
// Poll for events and return results
// ----------------------------------------------------------------------------

std::vector<PollResult> EventPoller::poll_once() {
    std::vector<PollResult> results;

    // If no fds to monitor, return empty results
    if (poll_fds_.empty()) {
        return results;
    }

    // Wait for events
    int ready = poll(&poll_fds_[0], poll_fds_.size(), POLL_TIMEOUT_MS);

    if (ready < 0) {
        if (errno == EINTR) {
            // Interrupted by signal, return empty results
            return results;
        }
        throw std::runtime_error("Poll failed");
    }

    if (ready == 0) {
        // Timeout, return empty results
        return results;
    }

    // Process all ready file descriptors
    for (size_t i = 0; i < poll_fds_.size() && ready > 0; ++i) {
        if (poll_fds_[i].revents != 0) {
            results.push_back(create_poll_result(poll_fds_[i]));
            ready--;
        }
    }

    return results;
}

PollResult EventPoller::create_poll_result(const struct pollfd& pfd) {
    PollResult result;
    result.fd = pfd.fd;

    // Check for each type of event separately
    result.can_read = (pfd.revents & PollEvents::READ) != 0;
    result.can_write = (pfd.revents & PollEvents::WRITE) != 0;
    result.has_error = (pfd.revents & PollEvents::ERROR) != 0;
    result.has_hup = (pfd.revents & PollEvents::HUP) != 0;

    return result;
}

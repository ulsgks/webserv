#include "Socket.hpp"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

#include "../utils/Log.hpp"

Socket::Socket(int port) : fd_(-1) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port);

    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set FD_CLOEXEC to prevent inheritance by child processes (e.g., CGI)
    fcntl(fd_, F_SETFD, FD_CLOEXEC);
}

Socket::~Socket() {
    if (fd_ != -1) {
        close_socket();
    }
}

Socket::Socket(const Socket& other) : fd_(-1) {
    *this = other;
}

Socket& Socket::operator=(const Socket& other) {
    if (this != &other) {
        if (fd_ != -1) {
            close_socket();
        }
        fd_ = dup(other.fd_);
        addr_ = other.addr_;
    }
    return *this;
}

void Socket::configure_socket() {
    // Allow reuse of address (useful for development)
    int opt = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    // Set non-blocking mode and FD_CLOEXEC for server socket
    if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    if (bind(fd_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0) {
        throw std::runtime_error(
            "Failed to bind socket on port " + Log::to_string(ntohs(addr_.sin_port)));
    }

    if (listen(fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

int Socket::accept_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(fd_, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        // Subject forbids errno checking after I/O operations
        // For non-blocking sockets, this is expected when no connection is available
        return -1;  // No connection available
    }

    // Set non-blocking mode for client socket
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
        close(client_fd);
        throw std::runtime_error("Failed to set client non-blocking mode");
    }

    // Set FD_CLOEXEC to prevent inheritance by child processes
    if (fcntl(client_fd, F_SETFD, FD_CLOEXEC) == -1) {
        close(client_fd);
        throw std::runtime_error("Failed to set FD_CLOEXEC on client socket");
    }

    return client_fd;
}

int Socket::get_fd() const {
    return fd_;
}

void Socket::close_socket() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <stdexcept>
#include <string>

#include <netinet/in.h>
#include <sys/socket.h>

class Socket {
   private:
    int fd_;                   // Socket file descriptor
    struct sockaddr_in addr_;  // Socket address structure

   public:
    Socket(int port);
    ~Socket();

    // Prevent copying of sockets
    Socket(const Socket& other);
    Socket& operator=(const Socket& other);

    void configure_socket();
    int accept_connection();

    // Utility methods
    int get_fd() const;
    void close_socket();
};

#endif

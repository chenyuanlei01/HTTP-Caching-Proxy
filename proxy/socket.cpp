#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

/**
 * @brief Default constructor that creates a new TCP socket
 * 
 * Creates a new socket with IPv4 address family and TCP protocol.
 * Sets the SO_REUSEADDR option to allow reusing local addresses.
 * @throws std::runtime_error if socket creation or option setting fails
 */
TcpSocket::TcpSocket() : socket_fd_(-1) {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
}

/**
 * @brief Constructor for creating a socket from an existing file descriptor
 * 
 * Used primarily by the accept() method to create a socket for a new client connection.
 * @param socket_fd Existing socket file descriptor
 * @param client_addr Socket address information of the client
 */
TcpSocket::TcpSocket(int socket_fd, struct sockaddr_in client_addr) 
    : socket_fd_(socket_fd), address_(client_addr) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    remote_address_ = std::string(ip_str);
}

/**
 * @brief Destructor that ensures the socket is properly closed
 */
TcpSocket::~TcpSocket() {
    close();
}

/**
 * @brief Binds the socket to the specified port on all available interfaces
 * 
 * Sets up the socket address structure with the specified port and binds
 * the socket to that address.
 * @param port The port number to bind to
 * @return true if binding was successful, false otherwise
 */
bool TcpSocket::bind(int port) {
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port);

    if (::bind(socket_fd_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
        return false;
    }
    return true;
}

/**
 * @brief Places the socket in a passive listening state
 * 
 * Marks the socket as a passive socket that will accept incoming connection requests.
 * @param backlog Maximum length of the queue of pending connections
 * @return true if the operation was successful, false otherwise
 */
bool TcpSocket::listen(int backlog) {
    if (::listen(socket_fd_, backlog) < 0) {
        return false;
    }
    return true;
}

/**
 * @brief Accepts an incoming connection attempt
 * 
 * Extracts the first connection request from the pending connections queue,
 * creates a new socket for it, and returns a shared pointer to a TcpSocket
 * that encapsulates it.
 * @return A shared pointer to a new TcpSocket object representing the accepted connection,
 *         or nullptr if the operation failed
 */
std::shared_ptr<ISocket> TcpSocket::accept() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_socket = ::accept(this->socket_fd_, (struct sockaddr*)&client_addr, &addr_len);
    
    if (client_socket < 0) {
        return nullptr;
    }

    return std::make_shared<TcpSocket>(client_socket, client_addr);
}

/**
 * @brief Initiates a connection to a remote host
 * 
 * Resolves the hostname to an IP address using gethostbyname() and attempts to
 * establish a connection to that address on the specified port.
 * @param host The hostname or IP address of the remote host
 * @param port The port number on the remote host
 * @return true if the connection was successful, false otherwise
 */
bool TcpSocket::connect(const std::string& host, int port) {
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        return false;
    }

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        return false;
    }
    return true;
}

/**
 * @brief Sends data over the socket
 * 
 * Transmits the provided data to the connected peer.
 * @param data Vector of bytes to send
 * @return Number of bytes sent, or -1 if an error occurred
 */
ssize_t TcpSocket::send(const std::vector<uint8_t>& data) {
    return ::send(socket_fd_, data.data(), data.size(), 0);
}

/**
 * @brief Receives data from the socket
 * 
 * Reads data from the socket into the provided buffer.
 * Resizes the buffer to match the number of bytes actually read.
 * @param buffer Vector to store the received data
 * @param max_size Maximum number of bytes to receive
 * @return Number of bytes received, 0 for end of stream, or -1 if an error occurred
 */
ssize_t TcpSocket::receive(std::vector<uint8_t>& buffer, size_t max_size) {
    buffer.resize(max_size);
    ssize_t bytes_read = ::recv(socket_fd_, buffer.data(), max_size, 0);
    if (bytes_read > 0) {
        buffer.resize(bytes_read);
    } else {
        buffer.clear();
    }
    return bytes_read;
}

/**
 * @brief Closes the socket
 * 
 * Closes the underlying file descriptor if it's valid and sets it to -1.
 */
void TcpSocket::close() {
    if (socket_fd_ != -1) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

/**
 * @brief Gets the remote peer's address
 * 
 * @return String representation of the remote IP address
 */
std::string TcpSocket::getRemoteAddress() const {
    return remote_address_;
}

/**
 * @brief Gets the socket file descriptor
 * 
 * @return The underlying socket file descriptor
 */
int TcpSocket::getSocketFd() const {
    return socket_fd_;
}

/**
 * @brief Shuts down the write side of the socket
 * 
 * Disables further send operations on the socket.
 */
void TcpSocket::shutdownWrite() {
    if (socket_fd_ != -1) {
        shutdown(socket_fd_, SHUT_WR);
    }
}
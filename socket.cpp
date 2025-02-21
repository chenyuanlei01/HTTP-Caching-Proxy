#include "socket.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

Socket::Socket() {
    socket_fd = -1;
    hostname = nullptr;
    port = nullptr;
    host_info_list = nullptr;
}

Socket::~Socket() {
    if (host_info_list != nullptr) {
        freeaddrinfo(host_info_list);
    }
    if (socket_fd != -1) {
        close(socket_fd);
    }
}

void ServerSocket::init_server(const char *port) {
    this->port = port;
    this->host_info.ai_family   = AF_UNSPEC;
    this->host_info.ai_socktype = SOCK_STREAM;
    this->host_info.ai_flags    = AI_PASSIVE;
    this->host_info.ai_protocol = 0;
    
    this->status = getaddrinfo(nullptr, this->port, &this->host_info, &this->host_info_list);

    // Create socket
    this->socket_fd = socket(this->host_info_list->ai_family, 
                      this->host_info_list->ai_socktype, 
                      this->host_info_list->ai_protocol);

    if (this->socket_fd < 0) {
        cerr << "[Error] failed to create socket" << endl;
        return;
    }

    // Set socket options to allow address reuse
    int opt = 1;
    this->status = setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (this->status == -1) {
        cerr << "[Error] failed to set socket option" << endl;
        return;
    }

    // Bind socket to address
    this->status = bind(this->socket_fd, this->host_info_list->ai_addr, this->host_info_list->ai_addrlen);
    if (this->status == -1) {
        cerr << "[Error] failed to bind socket" << endl;
        return;
    }

    // Start listening for connections, with a queue length of 100
    this->status = listen(this->socket_fd, 100);
    if (this->status == -1) {
        cerr << "[Error] failed to listen" << endl;
        return;
    }

    cout << "[Note] server is listening on port " << this->port << endl;
    return;
}

int ClientSocket::init_client(const char *hostname, const char *port) {
    this->hostname = hostname;
    this->port = port;
    this->host_info.ai_family   = AF_UNSPEC;
    this->host_info.ai_socktype = SOCK_STREAM;
    this->host_info.ai_flags    = AI_PASSIVE;
    this->host_info.ai_protocol = 0;
    
    this->status = getaddrinfo(this->hostname, this->port, &this->host_info, &this->host_info_list);
    if (this->status != 0) {
        cerr << "[Error] failed to get address info" << endl;
        return -1;
    }

    // Create socket
    this->socket_fd = socket(this->host_info_list->ai_family, 
                      this->host_info_list->ai_socktype, 
                      this->host_info_list->ai_protocol);

    if (this->socket_fd < 0) {
        cerr << "[Error] failed to create socket" << endl;
        return this->socket_fd;
    }

    // Connect to the server
    this->status = connect(this->socket_fd, this->host_info_list->ai_addr, this->host_info_list->ai_addrlen);
    if (this->status == -1) {
        cerr << "[Error] failed to connect to server" << endl;
        return this->socket_fd;
    }

    cout << "[Note] connected to " << this->hostname << ":" << this->port << endl;
    return this->socket_fd;
}

int ServerSocket::accept_connection() {
    // Create client address structure
    struct sockaddr_storage client_addr;  // Can store IPv4 or IPv6 address
    socklen_t client_len = sizeof(client_addr);
    
    // Accept connection
    int client_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_fd == -1) {
        cerr << "[Error] failed to accept connection" << endl;
        return client_fd;
    }

    // Get client information (optional, for logging)
    char client_ip[INET6_ADDRSTRLEN];  // Big enough to store IPv4 or IPv6 address
    int client_port;

    if (client_addr.ss_family == AF_INET) {
        // IPv4
        struct sockaddr_in *addr = (struct sockaddr_in*)&client_addr;
        inet_ntop(AF_INET, &(addr->sin_addr), client_ip, INET6_ADDRSTRLEN);
        client_port = ntohs(addr->sin_port);
    } else {
        // IPv6
        struct sockaddr_in6 *addr = (struct sockaddr_in6*)&client_addr;
        inet_ntop(AF_INET6, &(addr->sin6_addr), client_ip, INET6_ADDRSTRLEN);
        client_port = ntohs(addr->sin6_port);
    }

    cout << "[Info] accepted connection from " << client_ip 
         << ":" << client_port << endl;

    return client_fd;
}



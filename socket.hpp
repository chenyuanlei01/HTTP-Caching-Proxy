#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

class Socket {
protected:
    int status;              // Status code for socket operations
    int socket_fd;           // File descriptor for the socket
    const char *hostname;    // Target hostname for connections
    const char *port;        // Port number as string
    struct addrinfo host_info;      // Host address information
    struct addrinfo *host_info_list;  // Linked list of host address info

public:
    Socket();
    virtual ~Socket();
};

class ServerSocket : public Socket {
public:
    // Initialize server
    void init_server(const char *port);
    // Await a connection from client
    int accept_connection();
};

class ClientSocket : public Socket {
public:
    // Initialize client
    int init_client(const char *hostname, const char *port);
};

#endif // SOCKET_HPP 
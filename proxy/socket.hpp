#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <vector>
#include <memory>
#include <netinet/in.h>

// Constants
constexpr int BUFFER_SIZE = 8192;

/**
 * Socket interface - Abstract away socket operations for testability
 */
class ISocket {
public:
    virtual ~ISocket() = default;
    virtual bool bind(int port) = 0;
    virtual bool listen(int backlog) = 0;
    virtual std::shared_ptr<ISocket> accept() = 0;
    virtual bool connect(const std::string& host, int port) = 0;
    virtual ssize_t send(const std::vector<uint8_t>& data) = 0;
    virtual ssize_t receive(std::vector<uint8_t>& buffer, size_t max_size) = 0;
    virtual void close() = 0;
    virtual std::string getRemoteAddress() const = 0;
    virtual int getSocketFd() const = 0;
    virtual void shutdownWrite() = 0;
};

/**
 * TCP Socket implementation
 */
class TcpSocket : public ISocket {
private:
    int socket_fd_;
    struct sockaddr_in address_;
    std::string remote_address_;

public:
    TcpSocket(); // Constructor declaration
    explicit TcpSocket(int socket_fd, struct sockaddr_in client_addr); // Constructor declaration
    ~TcpSocket() override; // Destructor declaration

    bool bind(int port) override; // Method declaration
    bool listen(int backlog) override; // Method declaration
    std::shared_ptr<ISocket> accept() override; // Method declaration
    bool connect(const std::string& host, int port) override; // Method declaration
    ssize_t send(const std::vector<uint8_t>& data) override; // Method declaration
    ssize_t receive(std::vector<uint8_t>& buffer, size_t max_size) override; // Method declaration
    void close() override; // Method declaration
    std::string getRemoteAddress() const override; // Method declaration
    int getSocketFd() const override; // Method declaration
    void shutdownWrite();
};

#endif // SOCKET_HPP
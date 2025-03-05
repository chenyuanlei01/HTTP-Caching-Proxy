#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <pthread.h>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <ctime>
#include "socket.hpp"
#include "request.hpp"
#include "response.hpp"
#include "cache.hpp"
#include "log.hpp"

using namespace std;

// Singleton logger for the proxy
extern Log* proxy_logger;
// Singleton cache for the proxy
extern Cache* proxy_cache;

struct ThreadData {
    std::shared_ptr<ISocket> client_socket;
    string id;
    
};

class Handler {
private:
    // Helper methods for request processing
    static string generateUniqueID();
    static string getCurrentTimeStr();
    static bool processGetRequest(int client_fd, const Request& request, const string& id);
    static bool processPostRequest(int client_fd, const Request& request, const string& id);
    static bool processConnectRequest(int client_fd, const Request& request, const string& id);
    static bool forwardRequest(int client_fd, const Request& request, const string& id);
    static void sendErrorResponse(int client_fd, int status_code, const string& message, const string& id);
    static bool tunnelTraffic(int client_fd, int server_fd, const string& id);
    static bool sendAll(int fd, const void* data, size_t size);
public:
    // Create and detach a new thread for handling connection
    static bool create_connection_thread(std::shared_ptr<ISocket> client_socket, string id);

    // Handle client connection in a thread
    static void* handle_connection(void* arg);
};

#endif // HANDLER_HPP
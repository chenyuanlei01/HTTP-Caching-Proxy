#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <pthread.h>
#include <string>

using namespace std;

struct ThreadData {
    int client_fd;
    string id;
};

class Handler {
private:

public:
    // Create and detach a new thread for handling connection
    static bool create_connection_thread(int client_fd, string id);

    // Handle client connection in a thread
    static void* handle_connection(void* arg);
};

#endif // HANDLER_HPP 
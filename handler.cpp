#include "handler.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

bool Handler::create_connection_thread(int client_fd, string id) {
    ThreadData *thread_data = new ThreadData();
    thread_data->client_fd = client_fd;
    thread_data->id = id;

    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_connection, thread_data) != 0) {
        cerr << "[Error] failed to create thread" << endl;
        delete thread_data;
        close(client_fd);
        return false;
    }
    pthread_detach(thread);
    return true;
}

void* Handler::handle_connection(void* arg) {
    // 1. Convert argument type
    ThreadData* data = (ThreadData*)arg;
    cout << "[Note] handling connection " << data->id << endl;
    
    // 3. Main loop for handling client requests
    while (true) {
        // TODO: Read data from client

        // TODO: Process GET request

        // TODO: Process POST request

        // TODO: Process CONNECT request
        
    }
    
    // 4. Cleanup
    close(data->client_fd);
    delete data;
    
    return NULL;
}
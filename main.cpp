#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "socket.hpp"
#include "handler.hpp"

using namespace std;

int main() {
    ServerSocket proxy_server;
    proxy_server.init_server("12345");
    
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    string id = boost::uuids::to_string(uuid);
    
    while(true) {
        cout << "[Note] waiting for connection..." << endl;
        int client_fd = proxy_server.accept_connection();
        Handler::create_connection_thread(client_fd, id);
    }

    return 0;
}

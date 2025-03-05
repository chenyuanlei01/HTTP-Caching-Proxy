#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "socket.hpp"
#include "handler.hpp"
#include "log.hpp"
using namespace std;

int main() {
    try {
        system("mkdir -p ./");
        system("chmod 777 ./logs/");
        
        // Initialize logger and cache
        proxy_logger = new Log(LOG_FILE);
        proxy_cache = new Cache(1000);
        
        proxy_logger->write("(no-id): NOTE Proxy server started");
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return 1;
    }
    
    auto proxy_server = std::make_shared<TcpSocket>();
    proxy_server->bind(12345);
    proxy_server->listen(10);  // Allow up to 10 pending connections
    
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    string id = boost::uuids::to_string(uuid);

    std::shared_ptr<ISocket> client_socket = nullptr;
    
    while(true) {
        cout << "[Note] waiting for connection..." << endl;
        client_socket = proxy_server->accept();
        if (client_socket == nullptr) {
            std::cerr << "Failed to accept connection" << std::endl;
            proxy_logger->write("(no-id): ERROR Failed to accept connection");
            continue;
        }
        Handler::create_connection_thread(client_socket, id);
    }

    return 0;
}

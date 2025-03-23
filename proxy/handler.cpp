#include "handler.hpp"
#include "log.hpp"
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <poll.h>
#include <algorithm>
#include <fcntl.h>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <thread>

using namespace std;

extern boost::asio::thread_pool * global_thread_pool;

// Initialize the global logger and cache
Log* proxy_logger = nullptr; 
Cache* proxy_cache = nullptr;

// Generate a unique request ID
string Handler::generateUniqueID() {
    static int counter = 0;
    return to_string(++counter);
}

// Get current time string in UTC format
string Handler::getCurrentTimeStr() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    string time_str = asctime(gmtime(&now_time));
    time_str.erase(time_str.find('\n')); // Remove trailing newline
    return time_str;
}

/**
 * Ensures all data is sent through the socket
 * 
 * @param fd Socket file descriptor
 * @param data Pointer to data buffer
 * @param size Size of data to send
 * @return true if all data was sent successfully, false otherwise
 */
 bool Handler::sendAll(int fd, const void* data, size_t size) {
    size_t total_sent = 0;
    const char* data_ptr = static_cast<const char*>(data);
    
    while (total_sent < size) {
        ssize_t sent = send(fd, data_ptr + total_sent, size - total_sent, 0);
        if (sent <= 0) {
            if (errno == EINTR) {
                // Interrupted by signal, retry
                continue;
            }
            return false;  // Error occurred
        }
        total_sent += sent;
    }
    return true;
}

bool Handler::create_connection_thread(std::shared_ptr<ISocket> client_socket, string id) {
    ThreadData *thread_data = new ThreadData();
    thread_data->client_socket = client_socket;  // Store the shared_ptr directly
    thread_data->id = id;

    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_connection, thread_data) != 0) {
        cerr << "[Error] failed to create thread" << endl;
        proxy_logger->write("(no-id): ERROR failed to create thread");
        delete thread_data;
        // No need to manually close the socket - shared_ptr will handle it
        return false;
    }
    pthread_detach(thread);
    return true;
}

bool Handler::post_thread_pool(std::shared_ptr<ISocket> client_socket, string id) {
    if (global_thread_pool == nullptr) {
        cerr << "[Error] Thread pool not initialized" << endl;
        if (proxy_logger) proxy_logger->write("(no-id): ERROR Thread pool not initialized");
        return false;
    }
    
    ThreadData *thread_data = new ThreadData();
    thread_data->client_socket = client_socket;
    thread_data->id = id;

    boost::asio::post(*global_thread_pool, [thread_data]() {
        handle_connection(thread_data);
    });
    
    return true;
}

void* Handler::handle_connection(void* arg) {
    cerr << "[DEBUG] Starting handle_connection" << endl;
    // 1. Convert argument type
    ThreadData* data = (ThreadData*)arg;
    string id = data->id;

    std::shared_ptr<ISocket> client_socket = data->client_socket;
    int client_fd = client_socket->getSocketFd(); // Get FD only when needed
    
    cerr << "[Note] handling connection " << id << endl;
    
    try {
        // 2. Read data from client
        vector<uint8_t> buffer(BUFFER_SIZE);
        
        ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), 0);
        
        if (bytes_read < 0) {
            cerr << "[Error] recv() failed: " << strerror(errno) << " (errno: " << errno << ")" << endl;
            proxy_logger->write(id + ": ERROR Failed to read from client");
            close(client_fd);
            delete data;
            return NULL;
        } else if (bytes_read == 0) {
            proxy_logger->write(id + ": Client closed connection");
            close(client_fd);
            delete data;
            return NULL;
        }
        
        // Convert received data to string for request parsing
        string request_str(buffer.begin(), buffer.begin() + bytes_read);
        
        // 3. Parse the HTTP request
        Request request;
        try {
            request = Request(request_str);
            request.parse();
        } catch (const InvalidRequest& e) {
            proxy_logger->write(id + ": ERROR Invalid request format");
            sendErrorResponse(client_fd, 400, "Bad Request", id);
            close(client_fd);
            delete data;
            return NULL;
        }
        
        // 4. Log the received request
        string client_ip = client_socket->getRemoteAddress();
        string log_entry = id + ": \"" + request.get_line() + "\" from " + client_ip + " @ " + getCurrentTimeStr();
        proxy_logger->write(log_entry);
        
        // 5. Process the request based on its method
        bool success = false;
        string method = request.get_method();
        
        if (method == "GET") {
            success = processGetRequest(client_fd, request, id);
        } else if (method == "POST") {
            success = processPostRequest(client_fd, request, id);
        } else if (method == "CONNECT") {
            success = processConnectRequest(client_fd, request, id);
        } else {
            // Unsupported method
            proxy_logger->write(id + ": WARNING Unsupported method: " + method);
            sendErrorResponse(client_fd, 501, "Not Implemented", id);
        }
        
        if (!success) {
            proxy_logger->write(id + ": ERROR Request handling failed");
        }
    } catch (const exception& e) {
        proxy_logger->write(id + ": ERROR Exception: " + string(e.what()));
        sendErrorResponse(client_fd, 500, "Internal Server Error", id);
    }
    
    // 6. Cleanup
    cout << "[Note] closing connection " << id << endl;

    // Use the object method instead of raw file descriptor
    client_socket->shutdownWrite();  // Uses RAII approach

    // Set a longer timeout to ensure data is flushed
    usleep(300000);  // 300ms delay

    // No need to call close() - the shared_ptr will handle this
    
    delete data;
    return NULL;
}

bool Handler::processGetRequest(int client_fd, const Request& request, const string& id) {
    std::cerr << "[DEBUG] Starting processGetRequest: " << id << std::endl;
    string url = request.get_hostname() + request.get_uri();
    
    // Check if the request is in cache
    auto cached_entry = proxy_cache->get(url);
    
    if (!cached_entry) {
        proxy_logger->write(id + ": not in cache");
        return forwardRequest(client_fd, request, id);
    } else if (cached_entry->isExpired()) {
        // Fix: Convert time_point to time_t using to_time_t
        time_t expired_time = chrono::system_clock::to_time_t(cached_entry->expires_time);
        tm* tm_expired = gmtime(&expired_time);
        string expired_time_str = asctime(tm_expired);
        expired_time_str.erase(expired_time_str.find('\n'));
        
        proxy_logger->write(id + ": in cache, but expired at " + expired_time_str);
        
        // Check if we can validate
        if (!cached_entry->etag.empty() || !cached_entry->last_modified.empty()) {
            proxy_logger->write(id + ": in cache, requires validation");
            // We should revalidate - implement conditional GET
            return forwardRequest(client_fd, request, id);
        } else {
            // Cannot validate, need to re-fetch
            return forwardRequest(client_fd, request, id);
        }
    } else {
        proxy_logger->write(id + ": in cache, valid");
        
        // Serve from cache
        string response_line = cached_entry->response_line;
        proxy_logger->write(id + ": Responding \"" + response_line + "\"");
        
        // Build response from cache
        stringstream response_stream;
        response_stream << cached_entry->response_line << "\r\n";
        
        for (const auto& header : cached_entry->headers) {
            response_stream << header.first << ": " << header.second << "\r\n";
        }
        
        response_stream << "\r\n";
        string headers = response_stream.str();
        
        // Send headers
        if (!sendAll(client_fd, headers.c_str(), headers.size())) {
            proxy_logger->write(id + ": ERROR Failed to send cache response headers");
            return false;
        }
        
        // Send body
        if (!cached_entry->data.empty()) {
            if (!sendAll(client_fd, cached_entry->data.data(), cached_entry->data.size())) {
                proxy_logger->write(id + ": ERROR Failed to send cache response body");
                return false;
            }
            
            // Add a brief pause to ensure data is completely transmitted
            usleep(50000);  // 50ms delay, adjust if needed
        }
        
        proxy_logger->write(id + ": DEBUG Sent " + std::to_string(cached_entry->data.size()) + 
                   " bytes of cache data before closing");
        
        return true;
    }
}

bool Handler::processPostRequest(int client_fd, const Request& request, const string& id) {
    proxy_logger->write(id + ": NOTE Processing POST request");
    return forwardRequest(client_fd, request, id);
}

bool Handler::processConnectRequest(int client_fd, const Request& request, const string& id) {
    string hostname = request.get_hostname();
    string port = request.get_port();
    
    proxy_logger->write(id + ": NOTE Processing CONNECT to " + hostname + ":" + port);
    proxy_logger->write(id + ": Requesting \"" + request.get_line() + "\" from " + hostname);
    
    // Create a connection to the destination server
    auto server_socket = std::make_shared<TcpSocket>();
    if (!server_socket->connect(hostname, stoi(port))) {
        proxy_logger->write(id + ": ERROR Failed to connect to " + hostname + ":" + port);
        sendErrorResponse(client_fd, 502, "Bad Gateway", id);
        return false;
    }
    
    // Send 200 OK to client to establish tunnel
    string ok_response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    proxy_logger->write(id + ": Responding \"HTTP/1.1 200 Connection Established\"");
    
    if (!sendAll(client_fd, ok_response.c_str(), ok_response.size())) {
        proxy_logger->write(id + ": ERROR Failed to send 200 OK for CONNECT");
        return false;
    }
    
    // Tunnel traffic between client and server
    proxy_logger->write(id + ": NOTE Tunnel established, beginning data transfer");
    bool tunnel_result = tunnelTraffic(client_fd, server_socket->getSocketFd(), id);
    proxy_logger->write(id + ": Tunnel closed");
    
    return tunnel_result;
}

bool Handler::forwardRequest(int client_fd, const Request& request, const string& id) {
    // Add this at the beginning of the method
    if (!proxy_logger || !proxy_cache) {
        std::cerr << "Logger or cache not initialized" << std::endl;
        return false;
    }
    
    string hostname = request.get_hostname();
    if (hostname.empty()) {
        if (proxy_logger) proxy_logger->write(id + ": ERROR Empty hostname in request");
        sendErrorResponse(client_fd, 400, "Bad Request", id);
        return false;
    }
    
    // Rest of your method...
    string port = request.get_port();
    
    // Add null checks and better error handling
    auto server_socket = std::make_shared<TcpSocket>();
    if (!server_socket || !server_socket->getSocketFd()) {
        proxy_logger->write(id + ": ERROR Failed to create server socket");
        sendErrorResponse(client_fd, 500, "Internal Server Error", id);
        return false;
    }
    
    proxy_logger->write(id + ": Requesting \"" + request.get_line() + "\" from " + hostname);
    
    // Connect to origin server
    if (!server_socket->connect(hostname, stoi(port))) {
        proxy_logger->write(id + ": ERROR Failed to connect to " + hostname + ":" + port);
        sendErrorResponse(client_fd, 502, "Bad Gateway", id);
        return false;
    }
    
    // Forward the request to the origin server
    if (!sendAll(server_socket->getSocketFd(), request.get_request().c_str(), request.get_request().size())) {
        proxy_logger->write(id + ": ERROR Failed to send request to origin server");
        sendErrorResponse(client_fd, 502, "Bad Gateway", id);
        return false;
    }
    
    // Receive response from origin server
    vector<uint8_t> response_buffer;
    size_t total_bytes_read = 0;
    bool keep_reading = true;
    
    // Read the response headers first
    string response_str;
    char buf[BUFFER_SIZE];
    ssize_t bytes_read;
    
    proxy_logger->write(id + ": NOTE Beginning to receive response from origin server");
    
    while (keep_reading) {
        bytes_read = recv(server_socket->getSocketFd(), buf, BUFFER_SIZE, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                proxy_logger->write(id + ": ERROR Failed to read from origin server: " + std::string(strerror(errno)));
            } else {
                proxy_logger->write(id + ": NOTE Origin server closed connection during header read");
            }
            break;
        }
        
        response_str.append(buf, bytes_read);
        total_bytes_read += bytes_read;
        
        // Check if we've reached the end of headers
        size_t header_end = response_str.find("\r\n\r\n");
        if (header_end != string::npos) {
            break;
        }
    }
    
    if (total_bytes_read == 0) {
        proxy_logger->write(id + ": ERROR No response from origin server");
        sendErrorResponse(client_fd, 502, "Bad Gateway", id);
        return false;
    }
    
    // Parse the response to extract the response line
    size_t line_end = response_str.find("\r\n");
    string response_line = response_str.substr(0, line_end);
    proxy_logger->write(id + ": Received \"" + response_line + "\" from " + hostname);
    
    // Check if it's a 200 OK response to a GET request for caching
    bool is_cacheable = (request.get_method() == "GET" && response_str.find("HTTP/1.1 200") == 0);
    
    // Send the response headers to the client
    if (!sendAll(client_fd, response_str.c_str(), response_str.size())) {
        proxy_logger->write(id + ": ERROR Failed to forward response to client");
        return false;
    }
    
    // Parse Content-Length and check for chunked encoding
    size_t content_length = 0;
    bool is_chunked = false;

    // Find Content-Length header if present
    size_t content_length_pos = response_str.find("Content-Length: ");
    if (content_length_pos != string::npos) {
        size_t start = content_length_pos + 16; // Length of "Content-Length: "
        size_t end = response_str.find("\r\n", start);
        content_length = std::stoi(response_str.substr(start, end - start));
    }

    // Check for chunked encoding
    is_chunked = (response_str.find("Transfer-Encoding: chunked") != string::npos);

    // Calculate how much of the body we already read in the initial headers read
    size_t header_end = response_str.find("\r\n\r\n");
    size_t body_received = 0;
    if (header_end != string::npos) {
        body_received = response_str.length() - (header_end + 4);
    }

    if (header_end != string::npos && is_cacheable) {
        // Add body data from initial response to response_buffer
        response_buffer.insert(response_buffer.end(), 
                              response_str.begin() + header_end + 4,
                              response_str.end());
    }

    // Continue reading the response body until we've received all data
    while (keep_reading) {
        bytes_read = recv(server_socket->getSocketFd(), buf, BUFFER_SIZE, 0);
        
        if (bytes_read <= 0) {
            // Server closed connection or error
            keep_reading = false;
        } else {
            // Forward data to client
            if (!sendAll(client_fd, buf, bytes_read)) {
                proxy_logger->write(id + ": ERROR Failed to forward response body to client");
                return false;
            }
            
            // Cache data if needed
            if (is_cacheable) {
                response_buffer.insert(response_buffer.end(), buf, buf + bytes_read);
            }
            
            body_received += bytes_read;
            
            // Check if we have received the complete response body
            if (content_length > 0 && !is_chunked && body_received >= content_length) {
                // We've received all the data based on Content-Length
                keep_reading = false;
            }
            
            // For chunked encoding, look for the end chunk ("0\r\n\r\n")
            if (is_chunked && bytes_read >= 5) {
                // Look for end chunk marker at the end of the data
                if (memcmp(buf + bytes_read - 5, "0\r\n\r\n", 5) == 0) {
                    keep_reading = false;
                }
            }
        }
    }
    
    // Process for caching if it's a 200 OK GET response
    if (is_cacheable) {
        try {
            Response response(response_str);
            
            // Check if the response is cacheable
            if (response.is_no_store()) {
                proxy_logger->write(id + ": not cacheable because Cache-Control: no-store");
            } else {
                // Create a cache entry
                CacheEntry entry;
                entry.response_line = response_line;
                entry.data = response_buffer;
                
                // Copy headers one by one using the existing get_header method
                // Instead of using get_headers() which doesn't exist
                const std::vector<std::string> header_names = {
                    "Content-Type", "Content-Length", "ETag", "Last-Modified", 
                    "Expires", "Cache-Control", "Date"
                };

                for (const auto& header_name : header_names) {
                    std::string value = response.get_header(header_name);
                    if (!value.empty()) {
                        entry.headers[header_name] = value;
                    }
                }
                
                // Set expiration info
                entry.creation_time = chrono::system_clock::now();
                entry.expires_time = chrono::system_clock::from_time_t(response.get_expire_time());
                entry.requires_validation = response.needs_validation();
                entry.etag = response.get_etag();
                entry.last_modified = response.get_header("Last-Modified");
                
                // Add to cache
                string url = request.get_hostname() + request.get_uri();
                proxy_cache->put(url, entry);
                
                if (response.needs_validation()) {
                    proxy_logger->write(id + ": cached, but requires re-validation");
                } else if (response.get_expire_time() > 0) {
                    time_t expire_time = response.get_expire_time();
                    tm* tm_expire = gmtime(&expire_time);
                    string expire_time_str = asctime(tm_expire);
                    expire_time_str.erase(expire_time_str.find('\n'));
                    
                    proxy_logger->write(id + ": cached, expires at " + expire_time_str);
                }
            }
        } catch (const exception& e) {
            proxy_logger->write(id + ": WARNING Failed to process response for caching: " + string(e.what()));
        }
    }
    
    proxy_logger->write(id + ": Responding \"" + response_line + "\"");
    return true;
}

void Handler::sendErrorResponse(int client_fd, int status_code, const string& message, const string& id) {
    string status_line = "HTTP/1.1 " + to_string(status_code) + " " + message;
    string response = status_line + "\r\n"
                     "Content-Type: text/plain\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "Error: " + message;
    
    proxy_logger->write(id + ": Responding \"" + status_line + "\"");
    sendAll(client_fd, response.c_str(), response.size());
}

bool Handler::tunnelTraffic(int client_fd, int server_fd, const string& id) {
    // Set both sockets to non-blocking mode
    int client_flags = fcntl(client_fd, F_GETFL, 0);
    int server_flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, client_flags | O_NONBLOCK); // 让client_fd变成非阻塞的
    fcntl(server_fd, F_SETFL, server_flags | O_NONBLOCK); // 让server_fd变成非阻塞的
    
    int flag = 1; // 关闭 Nagle 算法，使小包数据立即发送，不等待数据合并
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    
    char buffer[BUFFER_SIZE];
    bool client_closed = false, server_closed = false;
    
    struct pollfd poll_fds[2];  // 创建一个pollfd结构体数组，用于监听client_fd和server_fd的可读事件
    poll_fds[0].fd = client_fd;
    poll_fds[0].events = POLLIN; // 监听POLLIN事件，即监听client_fd的可读事件
    poll_fds[1].fd = server_fd;
    poll_fds[1].events = POLLIN; // 监听POLLIN事件，即监听server_fd的可读事件
    
    while (!client_closed && !server_closed) {
        int res = poll(poll_fds, 2, 30000); // 30s timeout
        
        if (res < 0) {
            proxy_logger->write(id + ": ERROR Poll failed in tunnel");
            return false;
        }
        
        if (res == 0) {
            // Timeout
            continue;
        }
        
        // Check client -> server
        if (poll_fds[0].revents & POLLIN) { // 如果client_fd可读
            ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0); // 读取client_fd的数据
            
            if (bytes_read > 0) {
                if (!sendAll(server_fd, buffer, bytes_read)) { // 发送数据到server_fd
                    server_closed = true;
                }
            } else {
                client_closed = true;
            }
        }
        
        // Check server -> client
        if (poll_fds[1].revents & POLLIN) { // 如果server_fd可读
            ssize_t bytes_read = recv(server_fd, buffer, BUFFER_SIZE, 0); // 读取server_fd的数据
            
            if (bytes_read > 0) {
                if (!sendAll(client_fd, buffer, bytes_read)) { // 发送数据到client_fd
                    client_closed = true;
                }
            } else {
                server_closed = true;
            }
        }
        
        // Check for other events (errors or hangups)
        if ((poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) || 
            (poll_fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))) {
            proxy_logger->write(id + ": NOTE Tunnel connection terminated by peer");
            break;
        }
    }
    
    // Restore socket flags
    fcntl(client_fd, F_SETFL, client_flags);
    fcntl(server_fd, F_SETFL, server_flags);
    
    return true;
}
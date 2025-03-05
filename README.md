# 🌐 HTTP Caching Proxy

A high-performance HTTP proxy server with caching capabilities, built in C++ with a thread-per-connection model.

## ✨ Features

- 🔄 **HTTP Request Handling**: Supports GET, POST, and CONNECT methods
- 💾 **Caching**: Implements a thread-safe LRU cache with proper validation and expiration
- 🔒 **HTTPS Support**: Tunnels HTTPS connections via the CONNECT method
- 🧵 **Concurrency**: Uses a thread-per-connection model for handling multiple clients
- 📝 **Logging**: Comprehensive logging of all proxy activities

## 🏗️ Architecture

The proxy server is built with the following components:

- 🔌 **TcpSocket**: Handles network connections with proper socket management
- 🛠️ **Handler**: Processes HTTP requests and manages client-server communication
- 📦 **Cache**: Implements a thread-safe LRU caching mechanism
- 📨 **Request/Response**: Parses and manages HTTP messages
- 🔐 **Locks**: Provides RAII-style synchronization primitives

## 🚀 Quick Start

### Clone the Repository

```bash
git clone git@github.com:chenyuanlei01/HTTP-Caching-Proxy.git
cd HTTP-Caching-Proxy
```

### Using Docker

1. Build and run the proxy server:

```bash
sudo docker-compose up
```

2. Configure your browser or client to use the proxy at `http://localhost:12345/`

### Configure Browser Proxy Settings

For testing the web proxy with a browser, **Firefox** is recommended for its straightforward proxy configuration.

1. Open Network Settings: 
   - Navigate to `Preferences` → `General` → `Network Settings` (at the bottom of the page).

2. Set Manual Proxy Configuration: 
   - Select `Manual proxy configuration`.
   - Set the `HTTP Proxy` to your server address, e.g., `http://<server_ip>:12345/`.
   - Check the box to also use this proxy for `FTP` and `HTTPS`.

3. Handling "Proxy Server is Refusing Connections" Error:
   - This error may appear if the proxy server is not running yet.
   - Once the proxy server starts, the error should disappear.

## 📋 Implementation Details

### 💾 Caching

- Implements HTTP caching according to RFC 7234
- Cache-Control directive handling
- ETag and Last-Modified validation
- Expiration time calculation
- Conditional requests for validation

### 🔄 Connection Handling

- Main thread accepts connections
- New thread spawned for each client
- Thread parses HTTP request
- GET requests check cache first
- Forward uncached/expired requests to origin server
- CONNECT requests establish client-server tunnel

### 🔒 Thread Safety

- Reader-writer locks for cache
- Mutex locks for logging
- RAII-style lock guards to prevent deadlocks

## 👥 Contributors

Current Maintainers: Chenyuan Lei and Ankit Raj

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

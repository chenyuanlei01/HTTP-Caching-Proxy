#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <boost/beast.hpp>
#include <cstring>
#include <iostream>
#include <unordered_map>

/*
Sample POST Request:

POST /submit-form HTTP/1.1
Host: example.com:8080
User-Agent: TestAgent/1.0
Content-Type: application/json
Content-Length: 47
Accept: application/json
Connection: keep-alive

{
    "name": "John Doe",
    "age": 30
}
*/


/*
Sample GET Request:

GET / HTTP/1.1
Host: example.com
Connection: close
*/

/**
 * @brief Represents an HTTP request.
 * 
 * This class is responsible for storing and parsing HTTP requests.
 */
class Request {
private:
    std::string request;        // Complete request string
    std::string line;           // Request line
    std::string body;           // Request body
    std::string method;         // HTTP method (GET, POST, etc.)
    std::string uri;            // Request URI
    std::string port;           // Port number
    std::string hostname;       // Hostname
    std::unordered_map<std::string, std::string> headers; // Request headers

public:
    Request(); // Default constructor
    
    /**
     * @brief Constructor that initializes the request with a raw HTTP request string.
     * 
     * @param _request The raw HTTP request string.
     */
    Request(const std::string& _request) : 
        request(_request), 
        line(""), 
        body(""), 
        method(""), 
        uri(""), 
        port(""), 
        hostname("") {
        
        size_t pos = request.find("\r\n");
        if (pos != std::string::npos) {
            line = request.substr(0, pos);
        }
    }
    
    ~Request(); // Destructor

    /**
     * @brief Parses the HTTP request.
     * 
     * This method extracts the method, URI, headers, and body from the request string.
     * It throws an InvalidRequest exception if the request format is invalid.
     */
    void parse();

    /**
     * @brief Prints the details of the request.
     * 
     * This method outputs the request body, method, URI, hostname, and port to the console.
     */
    void print();

    // Getters for request details
    std::string get_request() const { return request; }
    std::string get_line() const { return line; }
    std::string get_body() const { return body; }
    std::string get_method() const { return method; }
    std::string get_uri() const { return uri; }
    std::string get_port() const { return port.empty() ? "80" : port; }
    std::string get_hostname() const { return hostname; }
    std::string get_header(const std::string& key) const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : "";
    }
};

/**
 * @brief Exception class for invalid HTTP requests.
 * 
 * This class is thrown when a request cannot be parsed correctly.
 */
class InvalidRequest : public std::exception {
public:
    virtual const char* what() const noexcept override {
        return "Invalid request";
    }
};

#endif // REQUEST_HPP

#include "request.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

// Default constructor implementation
Request::Request() : request(""), line(""), body(""), method(""), uri(""), port(""), hostname("") {}

// Destructor implementation 
Request::~Request() {}

/**
 * @brief Parses the HTTP request.
 * 
 * This method uses Boost.Beast to parse the HTTP request string stored in the `request` member.
 * It extracts the HTTP method, URI, headers, and body from the request.
 * If the request format is invalid, an InvalidRequest exception is thrown.
 */
void Request::parse() {
    try {
        // Create a Boost Beast HTTP request parser for string body
        http::request_parser<http::string_body> parser;
        beast::error_code ec; // Variable to hold error codes during parsing
        
        // Feed the raw HTTP request string into the parser
        parser.put(boost::asio::buffer(request), ec);
        
        if (ec) {
            std::cerr << "[ERROR] Failed to parse HTTP request: " << ec.message() << std::endl;
            throw InvalidRequest(); // Throw custom exception on error
        }

        // Get the parsed HTTP request object
        http::request<http::string_body> req = parser.get();

        // Extract the HTTP method (e.g., GET, POST)
        method = req.method_string().to_string(); 
        
        // Extract the requested URI (e.g., /index.html)
        uri = req.target().to_string();           

        // Construct the full request line (e.g., GET /index.html HTTP/1.1)
        line = method + " " + uri + " HTTP/" + 
               std::to_string(req.version() / 10) + "." + 
               std::to_string(req.version() % 10);

        // Extract the Host header if available
        if (req.base().find("Host") != req.base().end()) {
            hostname = req.base().at("Host").to_string();
        }

        // Default to port 80 if none is specified in the Host header
        port = "80"; 
        std::size_t colon_pos = hostname.find(':');
        
        // If a port is specified in the Host header, extract it
        if (colon_pos != std::string::npos) {
            port = hostname.substr(colon_pos + 1);
            hostname = hostname.substr(0, colon_pos); // Remove port from hostname
        }

        // Extract the request body (for POST, PUT methods, etc.)
        body = req.body();
        
    } catch (const std::exception& e) {
        // Catch any standard exceptions and log the error
        std::cerr << "[ERROR] Exception during request parsing: " << e.what() << std::endl;
        throw InvalidRequest(); // Re-throw as a custom InvalidRequest exception
    }
}

/**
 * @brief Prints the parsed HTTP request details.
 * 
 */
void Request::print() {
    std::cout << "Request Body: " << body << std::endl;
    std::cout << "Method: " << method << std::endl;
    std::cout << "URI: " << uri << std::endl;
    std::cout << "Host: " << hostname << std::endl;
    std::cout << "Port: " << port << std::endl;
}

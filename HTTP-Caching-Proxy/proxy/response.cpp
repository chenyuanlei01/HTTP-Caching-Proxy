#include "response.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <ctime>

class InvalidResponse : public std::exception {
    public:
        virtual const char* what() const noexcept {
            return "Invalid response";
        }
    };
/**
 * @brief Parse the raw HTTP response into structured components.
 * 
 * @param raw The raw HTTP response string.
 * 
 * This function extracts the status line, headers, and body from the raw response.
 * It fills the internal data members with the parsed data and processes caching rules.
 */
void Response::parse(const std::string& raw) {
    // Locate the end of headers (start of the body)
    size_t body_pos = raw.find("\r\n\r\n");
    if (body_pos == std::string::npos) {
        throw InvalidResponse();
    }

    // Split the headers and body
    std::string headers_str = raw.substr(0, body_pos);
    body_ = raw.substr(body_pos + 4);
    raw_response_ = raw;

    // Parse status line (first line of headers)
    std::istringstream header_stream(headers_str);
    std::string status_line;
    std::getline(header_stream, status_line);

    // Parse the status line (e.g., HTTP/1.1 200 OK)
    std::istringstream status_stream(status_line);
    status_stream >> version_ >> status_code_ >> std::ws;
    std::getline(status_stream, status_phrase_);

    // Parse all remaining headers
    std::string header_line;
    while (std::getline(header_stream, header_line) && !header_line.empty()) {
        size_t colon_pos = header_line.find(": ");
        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 2);
            headers_map[key] = value;

            // Parse specific headers
            if (key == "Content-Type") {
                content_type_ = value;
            } else if (key == "Content-Length") {
                content_length_ = std::stoi(value);
            } else if (key == "ETag") {
                etag_ = value;
            } else if (key == "Cache-Control") {
                cache_control_ = value;
                process_cache_control(value);
            } else if (key == "Transfer-Encoding" && value == "chunked") {
                is_chunked_ = true;
            } else if (key == "Date") {
                date_ = parse_time(value);
            } else if (key == "Last-Modified") {
                last_modified_ = parse_time(value);
                need_validate_ = true;
            } else if (key == "Expires") {
                expire_time_ = parse_time(value);
            }
        }
    }

    // Manage cache time and freshness
    manage_cache_time();
    validate_freshness();
}

/**
 * @brief Process cache control directives from the Cache-Control header.
 * 
 * @param cache_control_str The raw Cache-Control header value.
 * 
 * This function sets various caching-related flags and ages based on the directives.
 */
void Response::process_cache_control(const std::string& cache_control_str) {
    is_private_ = cache_control_str.find("private") != std::string::npos;
    is_no_store_ = cache_control_str.find("no-store") != std::string::npos;
    is_no_cache_ = cache_control_str.find("no-cache") != std::string::npos;
    is_revalidate_ = cache_control_str.find("must-revalidate") != std::string::npos;

    size_t pos;
    if ((pos = cache_control_str.find("s-maxage=")) != std::string::npos) {
        s_max_age_ = std::stol(cache_control_str.substr(pos + 9));
    }
    if ((pos = cache_control_str.find("max-age=")) != std::string::npos) {
        max_age_ = std::stol(cache_control_str.substr(pos + 8));
    }
}

/**
 * @brief Parse an HTTP date string into a time_t value.
 * 
 * @param time_str The date string (e.g., "Tue, 15 Nov 1994 08:12:31 GMT").
 * @return time_t The parsed time.
 */
time_t Response::parse_time(const std::string& time_str) {
    struct tm gmttime = {};
    strptime(time_str.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &gmttime);
    return timegm(&gmttime);
}

/**
 * @brief Calculate cache expiration and freshness based on cache directives.
 * 
 * This function computes the expiration time using max-age, s-maxage, or Expires header.
 */
void Response::manage_cache_time() {
    time_t now = time(NULL);
    double age = difftime(now, date_);

    if (s_max_age_ > 0) {
        expire_time_ = date_ + s_max_age_;
        is_fresh_ = s_max_age_ > age;
    } else if (max_age_ > 0) {
        expire_time_ = date_ + max_age_;
        is_fresh_ = max_age_ > age;
    } else if (expire_time_ > 0) {
        is_fresh_ = expire_time_ > now;
    } else {
        is_fresh_ = true; // Default to fresh if no explicit cache control is found
    }
    current_age_ = now - date_;
}

/**
 * @brief Determine if the response needs revalidation.
 * 
 * The need for validation depends on freshness and cache-control directives.
 */
void Response::validate_freshness() {
    need_validate_ = !is_fresh_;
    if (!need_validate_ && headers_map.find("Cache-Control") != headers_map.end()) {
        std::string cache_control = headers_map["Cache-Control"];
        if (cache_control.find("must-revalidate") != std::string::npos || 
            cache_control.find("no-cache") != std::string::npos) {
            need_validate_ = true;
        }
    }
}

/**
 * @brief Get the value of a specific HTTP header.
 * 
 * @param key The header name.
 * @return std::string The header value or an empty string if not found.
 */
std::string Response::get_header(const std::string& key) const {
    auto it = headers_map.find(key);
    return it != headers_map.end() ? it->second : "";
}
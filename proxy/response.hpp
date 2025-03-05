#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

// Custom exception for invalid responses
// class InvalidResponse : public std::exception {
// public:
//     virtual const char* what() const noexcept override {
//         return "Invalid response";
//     }
// };

/*
Example Response:

HTTP/1.1 200 OK
Date: Wed, 01 Mar 2025 12:34:56 GMT
Content-Type: text/html; charset=UTF-8
Content-Length: 1234
ETag: "abcdef12345"
Cache-Control: public, max-age=3600, s-maxage=7200, must-revalidate, no-cache, private
Last-Modified: Tue, 28 Feb 2025 10:00:00 GMT
Expires: Wed, 01 Mar 2025 13:34:56 GMT
Transfer-Encoding: chunked
Connection: keep-alive

<html>
<head>
    <title>Example Response</title>
</head>
<body>
    <h1>Hello, this is a sample response body!</h1>
</body>
</html>
*/

class Response {
private:
    // Core HTTP response components
    std::string version_;
    std::string status_code_;
    std::string status_phrase_;
    std::string body_;
    std::string raw_response_;
    std::unordered_map<std::string, std::string> headers_map;

    // Caching and transfer-related attributes
    std::string etag_;
    std::string cache_control_;
    std::string transfer_encoding_;
    std::string content_type_;
    std::string cache_mode;

    int content_length_ = -1;
    long max_age_ = -1;
    long s_max_age_ = -1;

    bool is_private_ = false;
    bool is_revalidate_ = false;
    bool is_no_cache_ = false;
    bool is_no_store_ = false;
    bool is_chunked_ = false;
    bool is_fresh_ = true;
    bool need_validate_ = true;

    // Time management
    time_t date_ = 0;
    time_t expire_time_ = 0;
    time_t current_age_ = 0;
    time_t last_modified_ = 0;

    // Helper methods
    void parse(const std::string& raw_response);
    void parse_headers(const std::string& headers_str);
    void process_cache_control(const std::string& cache_control_str);
    time_t parse_time(const std::string& time_str);
    void manage_cache_time();
    void validate_freshness();

public:
    // Constructors
    Response() = default;
    explicit Response(const std::string& raw_response) { parse(raw_response); }

    // Getters
    std::string get_version() const { return version_; }
    std::string get_status_code() const { return status_code_; }
    std::string get_status_phrase() const { return status_phrase_; }
    std::string get_body() const { return body_; }
    std::string get_raw_response() const { return raw_response_; }
    std::string get_etag() const { return etag_; }
    std::string get_cache_control() const { return cache_control_; }
    std::string get_transfer_encoding() const { return transfer_encoding_; }
    std::string get_content_type() const { return content_type_; }
    std::string get_header(const std::string& key) const;
    int get_content_length() const { return content_length_; }

    // Caching and validation flags
    bool is_private() const { return is_private_; }
    bool is_revalidate() const { return is_revalidate_; }
    bool is_no_cache() const { return is_no_cache_; }
    bool is_no_store() const { return is_no_store_; }
    bool is_chunked() const { return is_chunked_; }
    bool is_fresh() const { return is_fresh_; }
    bool needs_validation() const { return need_validate_; }

    // Time-related getters
    time_t get_date() const { return date_; }
    time_t get_expire_time() const { return expire_time_; }
    time_t get_current_age() const { return current_age_; }
    time_t get_last_modified() const { return last_modified_; }
    long get_max_age() const { return max_age_; }
    long get_s_max_age() const { return s_max_age_; }

    // Utility methods
    void set_raw_response(const std::string& raw_response) { raw_response_ = raw_response; }
    bool is_null() const { return raw_response_.empty(); }
};

#endif // RESPONSE_HPP

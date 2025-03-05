#ifndef LOG_HPP
#define LOG_HPP

#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>

extern const char* LOG_FILE; 

class FileRAII {
private:
    std::ofstream file_;
    
public:
    FileRAII(const std::string& path, std::ios_base::openmode mode = std::ios_base::out) {
        file_.open(path, mode);
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
    }
    
    ~FileRAII() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    // Prevent copying
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;
    
    // Allow access to the file
    std::ofstream& get() {
        return file_;
    }
    
    // Check if file is open
    bool is_open() const {
        return file_.is_open();
    }
};

class Log {
private:
    std::mutex log_mutex_;
    FileRAII log_file_;
    
public:
    Log(const std::string& path) : log_file_(path, std::ios_base::app) {}
    
    void write(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_file_.get() << message << std::endl;
        log_file_.get().flush();
    }
};

#endif // LOG_HPP
#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <mutex>
#include <shared_mutex>
#include <memory>
#include <atomic>

namespace utils {

/**
 * ScopedLock - Generic RAII mutex wrapper that automatically releases lock when going out of scope
 */
template <typename MutexType>
class ScopedLock {
private:
    MutexType& mutex_;
    bool locked_;

public:
    explicit ScopedLock(MutexType& mutex) : mutex_(mutex), locked_(true) {
        mutex_.lock();
    }

    ~ScopedLock() {
        if (locked_) {
            mutex_.unlock();
        }
    }

    // Prevent copying
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

    // Allow manual unlock
    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }
};

/**
 * ReaderLock - RAII reader lock for shared_mutex (read-only access)
 */
class ReaderLock {
private:
    std::shared_mutex& mutex_;
    bool locked_;

public:
    explicit ReaderLock(std::shared_mutex& mutex) : mutex_(mutex), locked_(true) {
        mutex_.lock_shared();
    }

    ~ReaderLock() {
        if (locked_) {
            mutex_.unlock_shared();
        }
    }

    // Prevent copying
    ReaderLock(const ReaderLock&) = delete;
    ReaderLock& operator=(const ReaderLock&) = delete;

    void unlock() {
        if (locked_) {
            mutex_.unlock_shared();
            locked_ = false;
        }
    }
};

/**
 * WriterLock - RAII writer lock for shared_mutex (exclusive access)
 */
class WriterLock {
private:
    std::shared_mutex& mutex_;
    bool locked_;

public:
    explicit WriterLock(std::shared_mutex& mutex) : mutex_(mutex), locked_(true) {
        mutex_.lock();
    }

    ~WriterLock() {
        if (locked_) {
            mutex_.unlock();
        }
    }

    // Prevent copying
    WriterLock(const WriterLock&) = delete;
    WriterLock& operator=(const WriterLock&) = delete;

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }
};

/**
 * ConnectionGuard - RAII socket connection management
 */
template <typename Socket>
class ConnectionGuard {
private:
    std::shared_ptr<Socket> socket_;

public:
    explicit ConnectionGuard(std::shared_ptr<Socket> socket) : socket_(socket) {}
    
    ~ConnectionGuard() {
        if (socket_) {
            socket_->close();
        }
    }

    // Prevent copying
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    
    // Allow moving
    ConnectionGuard(ConnectionGuard&& other) noexcept : socket_(std::move(other.socket_)) {
        other.socket_ = nullptr;
    }
    
    ConnectionGuard& operator=(ConnectionGuard&& other) noexcept {
        if (this != &other) {
            if (socket_) {
                socket_->close();
            }
            socket_ = std::move(other.socket_);
            other.socket_ = nullptr;
        }
        return *this;
    }

    // Release ownership without closing
    std::shared_ptr<Socket> release() {
        auto tmp = socket_;
        socket_ = nullptr;
        return tmp;
    }
    
    // Access the underlying socket
    Socket* operator->() const {
        return socket_.get();
    }
    
    // Check if socket is valid
    explicit operator bool() const {
        return static_cast<bool>(socket_);
    }
};

} // namespace utils

#endif // LOCKS_HPP
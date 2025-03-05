#ifndef CACHE_HPP
#define CACHE_HPP

#include <unordered_map>
#include <string>
#include <list>
#include <optional>
#include <chrono>
#include <shared_mutex>
#include <vector>
#include "utils/locks.hpp"

/**
 * Stores complete HTTP response data with caching metadata
 */
struct CacheEntry {
    std::vector<uint8_t> data;                          // Raw response body
    std::string response_line;                          // HTTP status line
    std::unordered_map<std::string, std::string> headers; // Response headers
    std::chrono::system_clock::time_point creation_time;  // When cached
    std::chrono::system_clock::time_point expires_time;   // When expires
    bool requires_validation;                            // Needs revalidation
    std::string etag;                                   // Entity tag
    std::string last_modified;                          // Modification time
    
    /**
     * Determines if this entry is no longer fresh according to HTTP caching rules
     * 
     * @return True if entry requires server validation before use
     */
    bool isExpired() const {
        return requires_validation || 
               (expires_time != std::chrono::system_clock::time_point() && 
                std::chrono::system_clock::now() > expires_time);
    }
};

/**
 * Thread-safe HTTP response cache with LRU eviction policy
 */
class Cache {
private:
    mutable std::shared_mutex cache_mutex_;  // Thread synchronization
    std::unordered_map<std::string, CacheEntry> cache_map_;  // Main storage
    std::list<std::string> access_order_;    // LRU tracking list
    std::unordered_map<std::string, std::list<std::string>::iterator> key_to_iterator_;  // For O(1) LRU updates
    size_t max_entries_;  // Maximum capacity

    /**
     * Removes the least recently used entry when cache reaches capacity
     */
    void evictOldest();
    
    /**
     * Updates the access tracking data structures when an entry is accessed
     * 
     * @param key The URL key being accessed
     */
    void updateAccessOrder(const std::string& key);

public:
    /**
     * Creates a new cache with specified capacity
     * 
     * @param max_entries Maximum number of responses to store before eviction
     */
    explicit Cache(size_t max_entries = 1000);
    
    /**
     * Retrieves a cached response if available
     * 
     * Thread-safe read operation that allows multiple concurrent readers.
     * 
     * @param key The URL to look up
     * @return The cached entry if found, or nullopt if not in cache
     */
    std::optional<CacheEntry> get(const std::string& key) const;
    
    /**
     * Stores a response in the cache
     * 
     * Thread-safe write operation that ensures exclusive access.
     * Handles eviction if needed when at capacity.
     * 
     * @param key The URL to store
     * @param value The response data to cache
     */
    void put(const std::string& key, const CacheEntry& value);
    
    /**
     * Explicitly removes an entry from the cache
     * 
     * Thread-safe write operation that ensures exclusive access.
     * 
     * @param key The URL to remove
     */
    void remove(const std::string& key);
    
    /**
     * Empties the entire cache
     * 
     * Thread-safe write operation that ensures exclusive access.
     */
    void clear();
    
    /**
     * Checks if a URL has a valid, non-expired cache entry
     * 
     * Thread-safe read operation that combines lookup and expiration check.
     * 
     * @param key The URL to check
     * @return True if entry exists and is not expired
     */
    bool isValid(const std::string& key) const;
    
    /**
     * Reports current number of entries in the cache
     * 
     * Thread-safe read operation.
     * 
     * @return Number of cached responses
     */
    size_t size() const;
};

#endif // CACHE_HPP
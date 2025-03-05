#include "cache.hpp"

// Constructor
Cache::Cache(size_t max_entries) : max_entries_(max_entries) {}

// Helper to evict oldest entry
void Cache::evictOldest() {
    if (!access_order_.empty()) {
        std::string oldest_key = access_order_.back();
        access_order_.pop_back();
        cache_map_.erase(oldest_key);
        key_to_iterator_.erase(oldest_key);
    }
}

// Update access order for LRU
void Cache::updateAccessOrder(const std::string& key) {
    auto it = key_to_iterator_.find(key);
    if (it != key_to_iterator_.end()) {
        access_order_.erase(it->second);
    }
    
    access_order_.push_front(key);
    key_to_iterator_[key] = access_order_.begin();
}

// Thread-safe cache read
std::optional<CacheEntry> Cache::get(const std::string& key) const {
    utils::ReaderLock lock(cache_mutex_);
    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Thread-safe cache write
void Cache::put(const std::string& key, const CacheEntry& value) {
    utils::WriterLock lock(cache_mutex_);
    if (cache_map_.size() >= max_entries_) {
        evictOldest();
    }
    cache_map_[key] = value;
    updateAccessOrder(key);
}

// Thread-safe cache remove
void Cache::remove(const std::string& key) {
    utils::WriterLock lock(cache_mutex_);
    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
        cache_map_.erase(it);
        auto order_it = key_to_iterator_.find(key);
        if (order_it != key_to_iterator_.end()) {
            access_order_.erase(order_it->second);
            key_to_iterator_.erase(order_it);
        }
    }
}

// Thread-safe cache clear
void Cache::clear() {
    utils::WriterLock lock(cache_mutex_);
    cache_map_.clear();
    access_order_.clear();
    key_to_iterator_.clear();
}

// Check if an entry is in cache and not expired
bool Cache::isValid(const std::string& key) const {
    auto entry = get(key);
    if (!entry) return false;
    return !entry->isExpired();
}

// Get cache size
size_t Cache::size() const {
    utils::ReaderLock lock(cache_mutex_);
    return cache_map_.size();
}

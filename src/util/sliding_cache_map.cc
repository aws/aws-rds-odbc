//  Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "sliding_cache_map.h"
#include "host_info.h"
#include "host_selector/round_robin_property.h"

template <typename K, typename V>
void SlidingCacheMap<K, V>::put(const K& key, const V& value) {
    put(key, value, DEFAULT_EXPIRATION_SEC);
}

template <typename K, typename V>
void SlidingCacheMap<K, V>::put(const K& key, const V& value, int sec_ttl) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point expiry_time =
        std::chrono::steady_clock::now() + std::chrono::seconds(sec_ttl);
    cache[key] = CacheEntry{value, expiry_time, sec_ttl};
}

template <typename K, typename V>
V SlidingCacheMap<K, V>::get(const K& key) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (auto itr = cache.find(key); itr != cache.end()) {
        CacheEntry& entry = itr->second;
        if (entry.expiry > now) {
            // Update TTL & Return value
            entry.expiry = now + std::chrono::seconds(entry.sec_ttl);
            return entry.value;
        }
        // Expired, remove from cache
        cache.erase(itr);
    }
    return {};
}

template <typename K, typename V>
bool SlidingCacheMap<K, V>::find(const K& key) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (auto itr = cache.find(key); itr != cache.end()) {
        CacheEntry& entry = itr->second;
        if (entry.expiry > now) {
            // Update TTL & Return found
            entry.expiry = now + std::chrono::seconds(entry.sec_ttl);
            return true;
        }
        // Expired, remove from cache
        cache.erase(itr);
    }
    return false;
}

template <typename K, typename V>
unsigned int SlidingCacheMap<K, V>::size() {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    for (auto itr = cache.begin(); itr != cache.end();) {
        if (itr->second.expiry < now) {
            itr = cache.erase(itr);
        } else {
            itr++;
        }
    }
    return cache.size();
}

template <typename K, typename V>
void SlidingCacheMap<K, V>::clear() {
    std::lock_guard<std::mutex> lock(cache_lock);
    cache.clear();
}

// Explicit Template Instantiations
template class SlidingCacheMap<std::string, std::string>;
template class SlidingCacheMap<std::string, std::shared_ptr<round_robin_property::RoundRobinClusterInfo>>;
template class SlidingCacheMap<std::string, std::vector<HostInfo>>;

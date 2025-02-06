// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0
// (GPLv2), as published by the Free Software Foundation, with the
// following additional permissions:
//
// This program is distributed with certain software that is licensed
// under separate terms, as designated in a particular file or component
// or in the license documentation. Without limiting your rights under
// the GPLv2, the authors of this program hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have included with the program.
//
// Without limiting the foregoing grant of rights under the GPLv2 and
// additional permission as to separately licensed software, this
// program is also subject to the Universal FOSS Exception, version 1.0,
// a copy of which can be found along with its FAQ at
// http://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see
// http://www.gnu.org/licenses/gpl-2.0.html.

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

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

#ifndef SLIDING_CACHE_MAP_H_
#define SLIDING_CACHE_MAP_H_

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

template <typename K, typename V>
class SlidingCacheMap {
public:
    SlidingCacheMap() = default;

    void put(const K& key, const V& value);
    void put(const K& key, const V& value, int sec_ttl);
    V get(const K& key);
    bool find(const K& key);
    unsigned int size();
    void clear();

private:
    struct CacheEntry {
        V value;
        std::chrono::steady_clock::time_point expiry;
        int sec_ttl;
    };
    const int DEFAULT_EXPIRATION_SEC = 600; // 600s = 10m
    std::unordered_map<K, CacheEntry> cache;
    std::mutex cache_lock;
};

#endif // SLIDING_CACHE_MAP_H_

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

#ifndef CACHE_MAP_H_
#define CACHE_MAP_H_

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

template <typename K, typename V>
class CacheMap {
public:
    CacheMap() = default;

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
    };
    const int DEFAULT_EXPIRATION_SEC = 600; // 600s = 10m
    std::unordered_map<K, CacheEntry> cache;
    std::mutex cache_lock;
};

#endif // CACHE_MAP_H_
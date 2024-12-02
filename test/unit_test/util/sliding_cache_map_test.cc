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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sliding_cache_map.h>

#include <string>
#include <thread>

namespace {
    const std::string cache_key_a("key_a");
    const std::string cache_key_b("key_b");
    const std::string cache_val_a("val_a");
    const std::string cache_val_b("val_b");
    const std::string cache_empty("");
    const int cache_exp_short(3);
    const int cache_exp_mid(5);
    const int cache_exp_long(6000);
}

class SlidingCacheMapTest : public testing::Test {
  protected:
    // Runs once per suite
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    // Runs per test case
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SlidingCacheMapTest, put_cache) {
    SlidingCacheMap<std::string, std::string> cache;
    cache.put(cache_key_a, cache_val_a);
    cache.put(cache_key_b, cache_val_b, cache_exp_long);
    EXPECT_STREQ(cache_val_a.c_str(), cache.get(cache_key_a).c_str());
    EXPECT_STREQ(cache_val_b.c_str(), cache.get(cache_key_b).c_str());
}

TEST_F(SlidingCacheMapTest, put_cache_update) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_EQ(0, cache.size());
    cache.put(cache_key_a, cache_val_a);
    cache.put(cache_key_b, cache_val_b, cache_exp_short);
    EXPECT_EQ(2, cache.size());
    EXPECT_STREQ(cache_val_b.c_str(), cache.get(cache_key_b).c_str());

    cache.put(cache_key_b, cache_val_b, cache_exp_long);
    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_mid));
    EXPECT_EQ(2, cache.size());
    EXPECT_STREQ(cache_val_b.c_str(), cache.get(cache_key_b).c_str());
}

TEST_F(SlidingCacheMapTest, get_cache_hit) {
    SlidingCacheMap<std::string, std::string> cache;
    cache.put(cache_key_a, cache_val_a);
    EXPECT_STREQ(cache_val_a.c_str(), cache.get(cache_key_a).c_str());
}

TEST_F(SlidingCacheMapTest, get_cache_miss) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_STREQ(cache_empty.c_str(), cache.get(cache_key_a).c_str());
}

TEST_F(SlidingCacheMapTest, get_cache_expire) {
    SlidingCacheMap<std::string, std::string> cache;
    cache.put(cache_key_a, cache_val_a, cache_exp_short);
    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_mid));
    EXPECT_STREQ(cache_empty.c_str(), cache.get(cache_key_a).c_str());
}

TEST_F(SlidingCacheMapTest, find_cache_miss) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_FALSE(cache.find("Missing"));
}

TEST_F(SlidingCacheMapTest, find_cache_hit) {
    SlidingCacheMap<std::string, std::string> cache;
    cache.put(cache_key_a, cache_val_a);
    EXPECT_TRUE(cache.find(cache_key_a));
}

TEST_F(SlidingCacheMapTest, cache_size) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_EQ(0, cache.size());
    cache.put(cache_key_a, cache_val_a);
    cache.put(cache_key_b, cache_val_b);
    EXPECT_EQ(2, cache.size());
}

TEST_F(SlidingCacheMapTest, cache_size_expire) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_EQ(0, cache.size());
    cache.put(cache_key_a, cache_val_a, cache_exp_short);
    cache.put(cache_key_b, cache_val_b, cache_exp_short);
    EXPECT_EQ(2, cache.size());
    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_mid));
    EXPECT_EQ(0, cache.size());
}

TEST_F(SlidingCacheMapTest, cache_clear) {
    SlidingCacheMap<std::string, std::string> cache;
    cache.put(cache_key_a, cache_val_a);
    cache.put(cache_key_b, cache_val_b);
    EXPECT_EQ(2, cache.size());
    cache.clear();
    EXPECT_EQ(0, cache.size());
}

TEST_F(SlidingCacheMapTest, ttl_update_find) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_EQ(0, cache.size());
    cache.put(cache_key_a, cache_val_a, cache_exp_mid);
    EXPECT_EQ(1, cache.size());

    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_short));

    // Touch to refresh TTL
    EXPECT_TRUE(cache.find(cache_key_a));

    // Sleep again combined with previous to expire original put
    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_short));

    // Should be valid due to earlier `find()` updates TTL
    EXPECT_EQ(1, cache.size());
    EXPECT_STREQ(cache_val_a.c_str(), cache.get(cache_key_a).c_str());
}

TEST_F(SlidingCacheMapTest, ttl_update_get) {
    SlidingCacheMap<std::string, std::string> cache;
    EXPECT_EQ(0, cache.size());
    cache.put(cache_key_a, cache_val_a, cache_exp_mid);
    EXPECT_EQ(1, cache.size());

    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_short));

    // Touch to refresh TTL
    EXPECT_STREQ(cache_val_a.c_str(), cache.get(cache_key_a).c_str());

    // Sleep again combined with previous to expire original put
    std::this_thread::sleep_for(std::chrono::seconds(cache_exp_short));

    // Should be valid due to earlier `get()` updates TTL
    EXPECT_EQ(1, cache.size());
    EXPECT_STREQ(cache_val_a.c_str(), cache.get(cache_key_a).c_str());
}

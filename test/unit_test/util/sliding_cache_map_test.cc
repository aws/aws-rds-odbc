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

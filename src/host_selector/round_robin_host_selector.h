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

#ifndef ROUNDROBIN_HOST_SELECTOR_H_
#define ROUNDROBIN_HOST_SELECTOR_H_

#include <mutex>
#include <vector>

#include "../host_info.h"
#include "../util/sliding_cache_map.h"
#include "host_selector.h"
#include "round_robin_property.h"

class RoundRobinHostSelector: public HostSelector {
public:
    HostInfo get_host(std::vector<HostInfo> hosts, bool is_writer,
        std::unordered_map<std::string, std::string> properties) override;
    static void set_round_robin_weight(std::vector<HostInfo> hosts, 
        std::unordered_map<std::string, std::string>& properties);
    static void clear_cache();

private:
    const int DEFAULT_WEIGHT = 1;
    const int NO_HOST_IDX = -1;

    static std::mutex cache_mutex;
    static SlidingCacheMap<std::string, std::shared_ptr<round_robin_property::RoundRobinClusterInfo>> round_robin_cache;

    virtual int convert_to_int(const std::string& str);

    virtual bool check_prop_change(const std::string& value, const std::string& prop_name,
        const std::unordered_map<std::string, std::string>& props);
    virtual void create_cache_entries(const std::vector<HostInfo>& hosts,
        const std::unordered_map<std::string, std::string>& props);
    virtual void update_cache(const std::vector<HostInfo>& hosts,
        const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& cluster_info);
    virtual void update_props_default_weight(
        const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& info,
        const std::unordered_map<std::string, std::string>& props);
    virtual void update_props_host_weight(
        const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& info,
        const std::unordered_map<std::string, std::string>& props);
};

#endif // ROUNDROBIN_HOST_SELECTOR_H_

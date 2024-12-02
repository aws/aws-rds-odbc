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

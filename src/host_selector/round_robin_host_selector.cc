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

#include "round_robin_host_selector.h"

// Initialize static members
std::mutex RoundRobinHostSelector::cache_mutex;
SlidingCacheMap<std::string, std::shared_ptr<round_robin_property::RoundRobinClusterInfo>> RoundRobinHostSelector::round_robin_cache;

void RoundRobinHostSelector::set_round_robin_weight(std::vector<HostInfo> hosts,
    std::unordered_map<std::string, std::string>& properties) {

    std::string host_weight_str;
    for (int i = 0; i < hosts.size(); i++) {
        host_weight_str += hosts.at(i).get_host();
        host_weight_str += ":";
        host_weight_str += std::to_string(hosts.at(i).get_weight());
        if (i < hosts.size() - 1) {
            host_weight_str += ",";
        }
    }
    properties[round_robin_property::HOST_WEIGHT_KEY] = host_weight_str;
}

void RoundRobinHostSelector::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    round_robin_cache.clear();
}

HostInfo RoundRobinHostSelector::get_host(std::vector<HostInfo> hosts, bool is_writer,
    std::unordered_map<std::string, std::string> properties) {

    std::lock_guard<std::mutex> lock(cache_mutex);

    std::vector<HostInfo> selection;
    selection.reserve(hosts.size());

    std::copy_if(hosts.begin(), hosts.end(), std::back_inserter(selection), [&is_writer](const HostInfo& host) {
        return host.is_host_up() && (is_writer ? host.is_host_writer() : true);
    });

    if (selection.empty()) {
        throw std::runtime_error("No available hosts found in list");
    }

    struct {
        bool operator()(const HostInfo& a, const HostInfo& b) const {
            return a.get_host() < b.get_host();
        }
    } host_name_sort;
    std::sort(selection.begin(), selection.end(), host_name_sort);

    create_cache_entries(selection, properties);
    std::string cluster_id_key = selection.at(0).get_host();
    std::shared_ptr<round_robin_property::RoundRobinClusterInfo> cluster_info = round_robin_cache.get(cluster_id_key);

    std::shared_ptr<HostInfo> last_host = cluster_info->last_host;
    int last_host_idx = NO_HOST_IDX;
    if (last_host) {
        for (int i = 0; i < selection.size(); i++) {
            if (selection.at(i).get_host() == last_host->get_host()) {
                last_host_idx = i;
            }
        }
    }

    int target_host_idx;
    if (cluster_info->weigth_counter > 0 && last_host_idx != NO_HOST_IDX) {
        target_host_idx = last_host_idx;
    } else {
        if (last_host_idx != NO_HOST_IDX && last_host_idx != selection.size() - 1) {
            target_host_idx = last_host_idx + 1;
        } else {
            target_host_idx = 0;
        }
        int weight = cluster_info->default_weight;
        if (auto itr = cluster_info->cluster_weight_map.find(selection.at(target_host_idx).get_host()); itr != cluster_info->cluster_weight_map.end()) {
            weight = itr->second;
        }
        cluster_info->weigth_counter = weight;
    }

    cluster_info->weigth_counter--;
    cluster_info->last_host = std::make_shared<HostInfo>(selection.at(target_host_idx));

    return selection.at(target_host_idx);
}

/**
 * This function is created to convert strings to integers.
 * An issue with using std::stoi() is that it will also parse floats/doubles
 * up until the decimal point, e.g. std::stoi("1.1") = 1
 * and is not a desirable case for our use
 */
int RoundRobinHostSelector::convert_to_int(const std::string& str) {
    std::istringstream stream(str);
    char c;
    int result;
    if (stream >> result) {
        if (stream.get(c)) {
            throw std::runtime_error("String is not a pure integer.");
        }
        return result;
    }
    throw std::runtime_error("Could not convert string to a pure integer.");
}

bool RoundRobinHostSelector::check_prop_change(const std::string& value, const std::string& prop_name,
    const std::unordered_map<std::string, std::string>& props) {

    if (props.find(prop_name) == props.end()) {
        return false;
    }
    const std::string& prop_value = props.at(prop_name);
    return value != prop_value;
}

void RoundRobinHostSelector::create_cache_entries(const std::vector<HostInfo>& hosts,
    const std::unordered_map<std::string, std::string>& props) {

    std::vector<HostInfo> hosts_with_cached_entry;
    hosts_with_cached_entry.reserve(round_robin_cache.size());
    std::copy_if(hosts.begin(), hosts.end(), std::back_inserter(hosts_with_cached_entry), [this](const HostInfo& host) {
        return RoundRobinHostSelector::round_robin_cache.find(host.get_host());
    });

    // Update cache entries, else create new cache
    if (hosts_with_cached_entry.empty()) {
        std::shared_ptr<round_robin_property::RoundRobinClusterInfo> cluster_info = std::make_shared<round_robin_property::RoundRobinClusterInfo>();
        update_props_default_weight(cluster_info, props);
        update_props_host_weight(cluster_info, props);
        update_cache(hosts, cluster_info);
    } else {        
        std::string cluster_id_key = hosts_with_cached_entry.at(0).get_host();
        std::shared_ptr<round_robin_property::RoundRobinClusterInfo> cluster_info = round_robin_cache.get(cluster_id_key);
        if (check_prop_change(cluster_info->last_default_weight_str, round_robin_property::DEFAULT_WEIGHT_KEY, props)) {
            cluster_info->default_weight = 1;
            update_props_default_weight(cluster_info, props);
        }
        if (check_prop_change(cluster_info->last_host_weight_str, round_robin_property::HOST_WEIGHT_KEY, props)) {
            cluster_info->last_host = nullptr;
            cluster_info->weigth_counter = 0;
            update_props_host_weight(cluster_info, props);
        }
        update_cache(hosts, cluster_info);
    }
};

void RoundRobinHostSelector::update_cache(const std::vector<HostInfo>& hosts,
    const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& cluster_info) {

    for (const HostInfo& host : hosts) {
        round_robin_cache.put(
            host.get_host(),
            cluster_info
        );
    }
}

void RoundRobinHostSelector::update_props_default_weight(
    const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& info,
    const std::unordered_map<std::string, std::string>& props) {

    int set_weight = DEFAULT_WEIGHT;
    if (auto itr = props.find(round_robin_property::DEFAULT_WEIGHT_KEY); itr != props.end()) {      
        try {
            set_weight = convert_to_int(itr->second);
            if (set_weight < DEFAULT_WEIGHT) {
                throw std::runtime_error("Invalid default host weight.");
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Default host weight not parsable as an integer.");
        }
        info->last_default_weight_str = itr->first;
    }
    info->default_weight = set_weight;
}

void RoundRobinHostSelector::update_props_host_weight(
    const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& info,
    const std::unordered_map<std::string, std::string>& props) {

    if (auto itr = props.find(round_robin_property::HOST_WEIGHT_KEY); itr != props.end()) {
        std::string host_weigths_str = itr->second;
        if (host_weigths_str.empty()) {
            info->cluster_weight_map.clear();
            info->last_host_weight_str = "";
            return;
        }
        std::istringstream stream(host_weigths_str);
        std::string token;
        std::vector<std::string> host_weight_groups;
        while (std::getline(stream, token, ',')) {
            host_weight_groups.push_back(token);
        }

        for (const std::string& host_weight_pair : host_weight_groups) {
            std::istringstream stream(host_weight_pair);
            std::string token;
            std::vector<std::string> host_weight_split;
            while (std::getline(stream, token, ':')) {
                host_weight_split.push_back(token);
            }
            if (host_weight_split.size() != 2) {
                throw std::runtime_error("Invalid format for host weight pair.");
            }

            std::string host_name = host_weight_split.at(0);
            std::string host_weight = host_weight_split.at(1);
            if (host_name.empty() || host_weight.empty()) {
                throw std::runtime_error("Host Name and/or host weight missing.");
            }

            try {
                int set_weight = convert_to_int(host_weight);
                if (set_weight < DEFAULT_WEIGHT) {
                    throw std::runtime_error("Invalid host weight.");
                }
                info->cluster_weight_map[host_name] = set_weight;
                info->last_host_weight_str = host_weigths_str;
            } catch (const std::exception& e) {
                throw std::runtime_error("Host weight not parsable as an integer.");
            }
        }
    }
}

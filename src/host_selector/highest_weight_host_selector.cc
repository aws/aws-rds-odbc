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

#include "highest_weight_host_selector.h"

HostInfo HighestWeightHostSelector::get_host(std::vector<HostInfo> hosts, bool is_writer, std::unordered_map<std::string, std::string>) {
    std::vector<HostInfo> eligible_hosts;
    eligible_hosts.reserve(hosts.size());

    std::copy_if(hosts.begin(), hosts.end(), std::back_inserter(eligible_hosts), [&is_writer](const HostInfo& host) {
        return host.is_host_up() && is_writer == host.is_host_writer();
    });

    if (eligible_hosts.empty()) {
        throw std::runtime_error("No eligible hosts found in list");
    }

    auto highest_weight_host = std::max_element(
        eligible_hosts.begin(),
        eligible_hosts.end(),
        [](const HostInfo& a, const HostInfo& b) {
            return a.get_weight() < b.get_weight();
        }
    );

    if (highest_weight_host == eligible_hosts.end()) {
        throw std::runtime_error("No eligible hosts found in list");
    }

    return *highest_weight_host;
}

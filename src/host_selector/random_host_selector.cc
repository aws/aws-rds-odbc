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

#include "random_host_selector.h"

#include <random>

HostInfo RandomHostSelector::get_host(std::vector<HostInfo> hosts, bool is_writer,
    std::unordered_map<std::string, std::string>) {

    std::vector<HostInfo> selection;
    selection.reserve(hosts.size());

    std::copy_if(hosts.begin(), hosts.end(), std::back_inserter(selection), [&is_writer](const HostInfo& host) {
        return host.is_host_up() && (is_writer ? host.is_host_writer() : true);
    });

    if (selection.empty()) {
        throw std::runtime_error("No available hosts found in list");
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, selection.size() - 1);

    int rand_idx = dis(gen);
    return selection[rand_idx];
}

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

    auto highest_weight_host =
        std::ranges::max_element(eligible_hosts, [](const HostInfo& a, const HostInfo& b) {
            return a.get_weight() < b.get_weight();
        });

    if (highest_weight_host == eligible_hosts.end()) {
        throw std::runtime_error("No eligible hosts found in list");
    }

    return *highest_weight_host;
}

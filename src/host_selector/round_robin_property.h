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

#include <memory>
#include <string>
#include <unordered_map>

#include "../host_info.h"

namespace round_robin_property {
    const std::string HOST_WEIGHT_KEY = "round_robin_host_weight_pairs";
    const std::string DEFAULT_WEIGHT_KEY = "round_robin_default_weight";

    using RoundRobinClusterInfo = struct RoundRobinClusterInfo {
        std::shared_ptr<HostInfo> last_host;
        std::unordered_map<std::string, int> cluster_weight_map;
        int default_weight = 1;
        int weigth_counter = 0;
        std::string last_default_weight_str;
        std::string last_host_weight_str;
    };
};

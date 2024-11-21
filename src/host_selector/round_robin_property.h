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

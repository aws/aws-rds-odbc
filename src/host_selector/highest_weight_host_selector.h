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

#ifndef HIGHEST_WEIGHT_HOST_SELECTOR_H_
#define HIGHEST_WEIGHT_HOST_SELECTOR_H_

#include "../host_info.h"
#include "host_selector.h"

class HighestWeightHostSelector: public HostSelector {
public:
    HostInfo get_host(std::vector<HostInfo> hosts, bool is_writer,
        std::unordered_map<std::string, std::string> properties) override;
};

#endif //HIGHEST_WEIGHT_HOST_SELECTOR_H_

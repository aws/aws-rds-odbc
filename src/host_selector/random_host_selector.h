// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RANDOM_HOST_SELECTOR_H_
#define RANDOM_HOST_SELECTOR_H_

#include "../host_info.h"
#include "host_selector.h"

class RandomHostSelector: public HostSelector {
public:
    HostInfo GetHost(std::vector<HostInfo> hosts, bool is_writer,
        std::unordered_map<std::string, std::string> properties) override;
};

#endif // RANDOM_HOST_SELECTOR_H_

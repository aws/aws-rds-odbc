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

#ifndef HOSTINFO_H_
#define HOSTINFO_H_

#include <memory>
#include <string>

#include "host_availability/host_availability_strategy.h"
#include "logger_wrapper.h"

enum HOST_STATE { UP, DOWN };

class HostInfo {
public:
    static constexpr uint64_t DEFAULT_WEIGHT = 100;
    static constexpr int NO_PORT = -1;

    HostInfo() = default;

    HostInfo(
        std::string host,
        int port,
        HOST_STATE state,
        bool is_writer,
        std::shared_ptr<HostAvailabilityStrategy> host_availability_strategy,
        uint64_t weight = DEFAULT_WEIGHT,
        SQL_TIMESTAMP_STRUCT last_updated_timestamp = SQL_TIMESTAMP_STRUCT()
    );

    ~HostInfo() = default;

    bool equal_host_port_pair(const HostInfo& hi) const;
    bool is_host_down() const;
    bool is_host_up() const;
    bool is_host_writer() const;

    void mark_as_writer(bool writer);

    std::string get_host() const;
    int get_port() const;
    std::string get_host_port_pair() const;
    uint64_t get_weight() const;
    SQL_TIMESTAMP_STRUCT get_last_updated_time() const;

    void set_host_state(HOST_STATE state);
    HOST_STATE get_host_state() const;

    void set_host_availability_strategy(std::shared_ptr<HostAvailabilityStrategy> new_host_availability_strategy);
    std::shared_ptr<HostAvailabilityStrategy> get_host_availability_strategy() const;

    std::string session_id;
    std::string replica_lag;

private:
    std::string host_port_separator = ":";
    std::string host;
    int port = NO_PORT;
    uint64_t weight = DEFAULT_WEIGHT;
    SQL_TIMESTAMP_STRUCT last_updated_timestamp;

    HOST_STATE host_state;
    bool is_writer;

    std::shared_ptr<HostAvailabilityStrategy> host_availability_strategy;
};

#endif /* HOSTINFO_H_ */

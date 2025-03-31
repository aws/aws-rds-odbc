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

#ifdef WIN32
    #include <windows.h>
#endif
#include <sqltypes.h>

#include "host_availability/host_availability_strategy.h"

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

    bool EqualHostPortPair(const HostInfo& hi) const;
    bool IsHostDown() const;
    bool IsHostUp() const;
    bool IsHostWriter() const;

    void MarkAsWriter(bool writer);

    std::string GetHost() const;
    int GetPort() const;
    std::string GetHostPortPair() const;
    uint64_t GetWeight() const;
    SQL_TIMESTAMP_STRUCT GetLastUpdatedTime() const;

    void SetHostState(HOST_STATE state);
    HOST_STATE GetHostState() const;

    void SetHostAvailabilityStrategy(std::shared_ptr<HostAvailabilityStrategy> new_host_availability_strategy);
    std::shared_ptr<HostAvailabilityStrategy> GetHostAvailabilityStrategy() const;

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

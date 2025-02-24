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

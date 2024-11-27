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
        uint64_t weight = DEFAULT_WEIGHT
    );

    ~HostInfo() = default;

    bool equal_host_port_pair(const HostInfo& hi) const;
    bool is_host_down() const;
    bool is_host_up() const;
    bool is_host_writer() const;

    std::string get_host_port_pair() const;
    std::string get_host() const;
    HOST_STATE get_host_state() const;
    uint64_t get_weight() const;

    int get_port() const;
    void mark_as_writer(bool writer);
    void set_host_state(HOST_STATE state);

    std::shared_ptr<HostAvailabilityStrategy> get_host_availability_strategy() const;
    void set_host_availability_strategy(std::shared_ptr<HostAvailabilityStrategy> new_host_availability_strategy);

    std::string session_id;
    std::string last_updated;
    std::string replica_lag;
private:
    std::string host_port_separator = ":";
    std::string host;
    int port = NO_PORT;
    uint64_t weight = DEFAULT_WEIGHT;

    HOST_STATE host_state;
    bool is_writer;

    std::shared_ptr<HostAvailabilityStrategy> host_availability_strategy;
};

#endif /* HOSTINFO_H_ */

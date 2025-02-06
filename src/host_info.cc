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

#include "host_info.h"

HostInfo::HostInfo(std::string host, int port, HOST_STATE state, bool is_writer, std::shared_ptr<HostAvailabilityStrategy> host_availability_strategy, uint64_t weight, SQL_TIMESTAMP_STRUCT last_updated_timestamp) :
    host { std::move(host) },
    port { port },
    host_state { state },
    is_writer { is_writer },
    host_availability_strategy{ std::move(host_availability_strategy) },
    weight { weight },
    last_updated_timestamp { SQL_TIMESTAMP_STRUCT(last_updated_timestamp) }
{
}

/**
 * Returns the host.
 *
 * @return the host
 */
std::string HostInfo::get_host() const {
    return host;
}

/**
 * Returns the port.
 *
 * @return the port
 */
int HostInfo::get_port() const {
    return port;
}

/**
 * Returns the weight
 *
 * @return the weight
 */
uint64_t HostInfo::get_weight() const {
    return weight;
}

SQL_TIMESTAMP_STRUCT HostInfo::get_last_updated_time() const {
    return last_updated_timestamp;
}

/**
 * Returns a host:port representation of this host.
 *
 * @return the host:port representation of this host
 */
std::string HostInfo::get_host_port_pair() const {
    return get_host() + host_port_separator + std::to_string(get_port());
}

bool HostInfo::equal_host_port_pair(const HostInfo& hi) const {
    return get_host_port_pair() == hi.get_host_port_pair();
}

HOST_STATE HostInfo::get_host_state() const {
    return host_state;
}

void HostInfo::set_host_state(HOST_STATE state) {
    host_state = state;
}

bool HostInfo::is_host_up() const {
    return host_state == UP;
}

bool HostInfo::is_host_down() const {
    return host_state == DOWN;
}

bool HostInfo::is_host_writer() const {
    return is_writer;
}

void HostInfo::mark_as_writer(bool writer) {
    is_writer = writer;
}

std::shared_ptr<HostAvailabilityStrategy> HostInfo::get_host_availability_strategy() const {
    return host_availability_strategy;
}

void HostInfo::set_host_availability_strategy(std::shared_ptr<HostAvailabilityStrategy> new_host_availability_strategy) {
    host_availability_strategy = std::move(new_host_availability_strategy);
}

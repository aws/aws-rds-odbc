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

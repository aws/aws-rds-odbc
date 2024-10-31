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

HOST_INFO::HOST_INFO(std::string host, int port, HOST_STATE state, bool is_writer, const HostAvailabilityStrategy& hostAvailabilityStrategy)
    : host{ host }, port{ port }, host_state{ state }, is_writer{ is_writer }, hostAvailabilityStrategy { hostAvailabilityStrategy }
{
}

HOST_INFO::~HOST_INFO() {}

/**
 * Returns the host.
 *
 * @return the host
 */
const std::string HOST_INFO::get_host() const {
    return host;
}

/**
 * Returns the port.
 *
 * @return the port
 */
int HOST_INFO::get_port() const {
    return port;
}

/**
 * Returns a host:port representation of this host.
 *
 * @return the host:port representation of this host
 */
const std::string HOST_INFO::get_host_port_pair() const {
    return get_host() + HOST_PORT_SEPARATOR + std::to_string(get_port());
}

bool HOST_INFO::equal_host_port_pair(HOST_INFO& hi) const {
    return get_host_port_pair() == hi.get_host_port_pair();
}

HOST_STATE HOST_INFO::get_host_state() const {
    return host_state;
}

void HOST_INFO::set_host_state(HOST_STATE state) {
    host_state = state;
}

bool HOST_INFO::is_host_up() const {
    return host_state == UP;
}

bool HOST_INFO::is_host_down() const {
    return host_state == DOWN;
}

bool HOST_INFO::is_host_writer() const {
    return is_writer;
}
void HOST_INFO::mark_as_writer(bool writer) {
    is_writer = writer;
}

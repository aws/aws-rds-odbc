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

#ifndef CLUSTER_TOPOLOGY_INFO_H_
#define CLUSTER_TOPOLOGY_INFO_H_

#include "host_info.h"

#include <ctime>
#include <set>
#include <vector>

#include "../util/logger_wrapper.h"

// This class holds topology information for one cluster.
// Cluster topology consists of an instance endpoint, a set of nodes in the cluster,
// the type of each node in the cluster, and the status of each node in the cluster.
class ClusterTopologyInfo {
public:
    ClusterTopologyInfo();
    ClusterTopologyInfo(const ClusterTopologyInfo& src_info); //copy constructor
    virtual ~ClusterTopologyInfo();

    void add_host(const std::shared_ptr<HostInfo>& host_info);
    size_t total_hosts();
    size_t num_readers(); // return number of readers in the cluster
    std::time_t time_last_updated() const;

    std::shared_ptr<HostInfo> get_writer();
    std::shared_ptr<HostInfo> get_next_reader();
    // TODO(yuenhcol) - Ponder if the get_reader below is needed. In general user of this should not need to deal with indexes.
    // One case that comes to mind, if we were to try to do a random shuffle of readers or hosts in general like JDBC driver
    // we could do random shuffle of host indices and call the get_reader for specific index in order we wanted.
    std::shared_ptr<HostInfo> get_reader(int i);
    std::vector<std::shared_ptr<HostInfo>> get_writers();
    std::vector<std::shared_ptr<HostInfo>> get_readers();

private:
    int current_reader = -1;
    std::time_t last_updated;
    std::set<std::string> down_hosts; // maybe not needed, HostInfo has is_host_down() method
    //std::vector<HostInfo*> hosts;
    std::shared_ptr<HostInfo> last_used_reader;  // TODO(yuenhcol) perhaps this overlaps with current_reader and is not needed

    // TODO(yuenhcol) - can we do without pointers -
    // perhaps ok for now, we are using copies ClusterTopologyInfo returned by get_topology and get_cached_topology from TOPOLOGY_SERVICE.
    // However, perhaps smart shared pointers could be used.
    std::vector<std::shared_ptr<HostInfo>> writers;
    std::vector<std::shared_ptr<HostInfo>> readers;

    std::shared_ptr<HostInfo> get_last_used_reader();
    void set_last_used_reader(const std::shared_ptr<HostInfo>& reader);
    void mark_host_down(const std::shared_ptr<HostInfo>& host);
    void mark_host_up(const std::shared_ptr<HostInfo>& host);
    std::set<std::string> get_down_hosts();
    void update_time();

    friend class TOPOLOGY_SERVICE;
};

#endif // CLUSTERTOPOLOGYINFO_H_

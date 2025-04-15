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

#include "cluster_topology_info.h"

#include <sstream>
#include <stdexcept>

#include <glog/logging.h>

/**
  Initialize and return random number.

  Returns random number.
 */
static int get_random_number() {
    std::srand(static_cast<unsigned int>(time(nullptr)));
    return rand();
}

ClusterTopologyInfo::ClusterTopologyInfo() {
    DLOG(INFO) << "Constructor";
    UpdateTime();
}

// copy constructor
ClusterTopologyInfo::ClusterTopologyInfo(const ClusterTopologyInfo& src_info)
    : current_reader{src_info.current_reader},
      last_updated{src_info.last_updated},
      last_used_reader{src_info.last_used_reader} {
    for (const auto& host_info_source : src_info.writers) {
        writers.push_back(std::make_shared<HostInfo>(*host_info_source)); //default copy
    }
    for (const auto& host_info_source : src_info.readers) {
        readers.push_back(std::make_shared<HostInfo>(*host_info_source)); //default copy
    }
}

ClusterTopologyInfo::~ClusterTopologyInfo() {
    DLOG(INFO) << "Deconstructor";
    for (auto p : writers) {
        p.reset();
    }
    writers.clear();

    for (auto p : readers) {
        p.reset();
    }
    readers.clear();
}

void ClusterTopologyInfo::AddHost(const std::shared_ptr<HostInfo>& host_info) {
    host_info->IsHostWriter() ? writers.push_back(host_info) : readers.push_back(host_info);
    UpdateTime();
    DLOG(INFO) << (host_info->IsHostWriter() ? "Writer" : "Reader" ) << ", " << host_info->GetHostPortPair() << " added to cluster topology.";
}

size_t ClusterTopologyInfo::TotalHosts() {
    return writers.size() + readers.size();
}

size_t ClusterTopologyInfo::NumReaders() {
    return readers.size();
}

std::time_t ClusterTopologyInfo::TimeLastUpdated() const {
    return last_updated;
}

// TODO(yuenhocol) harmonize time function across objects so the times are comparable
void ClusterTopologyInfo::UpdateTime() {
    last_updated = time(nullptr);
}

std::shared_ptr<HostInfo> ClusterTopologyInfo::GetWriter() {
    if (writers.empty()) {
        LOG(ERROR) << "No writer available in cluster topology.";
        throw std::runtime_error("No writer available");
    }

    return writers[0];
}

std::shared_ptr<HostInfo> ClusterTopologyInfo::GetNextReader() {
    size_t num_readers = readers.size();
    if (readers.empty()) {
        LOG(ERROR) << "No reader available in cluster topology.";
        throw std::runtime_error("No reader available");
    }

    if (current_reader == -1) { // initialize for the first time
        current_reader = get_random_number() % num_readers;
    }
    else if (current_reader >= num_readers) {
        // adjust the current reader in case topology was refreshed.
        current_reader = (current_reader) % num_readers;
    }
    else {
        current_reader = (current_reader + 1) % num_readers;
    }

    return readers[current_reader];
}

std::shared_ptr<HostInfo> ClusterTopologyInfo::GetReader(int i) {
    if (i < 0 || i >= readers.size()) {
        throw std::runtime_error("No reader available at index " + std::to_string(i));
    }

    return readers[i];
}

std::vector<std::shared_ptr<HostInfo>> ClusterTopologyInfo::GetReaders() {
    return readers;
}

std::vector<std::shared_ptr<HostInfo>> ClusterTopologyInfo::GetWriters() {
    return writers;
}

std::vector<std::shared_ptr<HostInfo>> ClusterTopologyInfo::GetHosts() {
    std::vector<std::shared_ptr<HostInfo>> instances(writers);
    instances.insert(instances.end(), readers.begin(), readers.end());

    return instances;
}

std::string ClusterTopologyInfo::LogTopology(const std::shared_ptr<ClusterTopologyInfo>& topology) {
    std::stringstream topology_str;
    if (topology->GetHosts().empty()) {
        topology_str << "<empty topology>";
        return topology_str.str();
    }

    for (const auto& host : topology->GetHosts()) {
        topology_str << "\n\t" << host;
    }

    return topology_str.str();
}

std::string ClusterTopologyInfo::LogTopology(const std::vector<HostInfo>& topology) {
    std::stringstream topology_str;
    if (topology.empty()) {
        topology_str << "<empty topology>";
        return topology_str.str();
    }

    for (const auto& host : topology) {
        topology_str << "\n\t" << host;
    }

    return topology_str.str();
}

std::shared_ptr<HostInfo> ClusterTopologyInfo::get_last_used_reader() {
    return last_used_reader;
}

void ClusterTopologyInfo::set_last_used_reader(const std::shared_ptr<HostInfo>& reader) {
    last_used_reader = std::move(reader);
}

void ClusterTopologyInfo::mark_host_down(const std::shared_ptr<HostInfo>& host) {
    host->SetHostState(DOWN);
    down_hosts.insert(host->GetHostPortPair());
}

void ClusterTopologyInfo::mark_host_up(const std::shared_ptr<HostInfo>& host) {
    host->SetHostState(UP);
    down_hosts.erase(host->GetHostPortPair());
}

std::set<std::string> ClusterTopologyInfo::get_down_hosts() {
    return down_hosts;
}

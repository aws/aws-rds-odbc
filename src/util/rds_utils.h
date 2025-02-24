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

#ifndef RDS_UTILS_H_
#define RDS_UTILS_H_

#include <regex>

class RdsUtils {
   public:
    static bool is_dns_pattern_valid(const std::string& host);
    static bool is_rds_dns(const std::string& host);
    static bool is_rds_cluster_dns(const std::string& host);
    static bool is_rds_proxy_dns(const std::string& host);
    static bool is_rds_writer_cluster_dns(const std::string& host);
    static bool is_rds_reader_cluster_dns(const std::string& host);
    static bool is_rds_custom_cluster_dns(const std::string& host);
    static bool is_ipv4(const std::string& host);
    static bool is_ipv6(const std::string& host);

    static std::string get_rds_cluster_host_url(const std::string& host);
    static std::string get_rds_cluster_id(const std::string& host);
    static std::string get_rds_instance_host_pattern(const std::string& host);
    static std::string get_rds_instance_id(const std::string& host);
    static std::string get_rds_region(const std::string& host);
};

#endif

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

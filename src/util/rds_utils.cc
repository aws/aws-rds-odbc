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

#include "rds_utils.h"
#include "string_helper.h"

namespace {
    const std::regex AURORA_DNS_PATTERN(
        R"#((.+)\.(proxy-|cluster-|cluster-ro-|cluster-custom-)?([a-zA-Z0-9]+\.([a-zA-Z0-9\-]+)\.rds\.amazonaws\.com))#",
        std::regex_constants::icase);
    const std::regex AURORA_PROXY_DNS_PATTERN(R"#((.+)\.(proxy-)+([a-zA-Z0-9]+\.[a-zA-Z0-9\-]+\.rds\.amazonaws\.com))#", std::regex_constants::icase);
    const std::regex AURORA_CLUSTER_PATTERN(R"#((.+)\.(cluster-|cluster-ro-)+([a-zA-Z0-9]+\.[a-zA-Z0-9\-]+\.rds\.amazonaws\.com))#",
                                            std::regex_constants::icase);
    const std::regex AURORA_WRITER_CLUSTER_PATTERN(R"#((.+)\.(cluster-)+([a-zA-Z0-9]+\.[a-zA-Z0-9\-]+\.rds\.amazonaws\.com))#",
                                                   std::regex_constants::icase);
    const std::regex AURORA_READER_CLUSTER_PATTERN(R"#((.+)\.(cluster-ro-)+([a-zA-Z0-9]+\.[a-zA-Z0-9\-]+\.rds\.amazonaws\.com))#",
                                                   std::regex_constants::icase);
    const std::regex AURORA_CUSTOM_CLUSTER_PATTERN(R"#((.+)\.(cluster-custom-)+([a-zA-Z0-9]+\.[a-zA-Z0-9\-]+\.rds\.amazonaws\.com))#",
                                                   std::regex_constants::icase);
    const std::regex AURORA_CHINA_DNS_PATTERN(
        R"#((.+)\.(proxy-|cluster-|cluster-ro-|cluster-custom-)?([a-zA-Z0-9]+\.(rds\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+\.rds)\.amazonaws\.com\.cn))#",
        std::regex_constants::icase);
    const std::regex AURORA_CHINA_PROXY_DNS_PATTERN(
        R"#((.+)\.(proxy-)+([a-zA-Z0-9]+\.(rds\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+\.rds)\.amazonaws\.com\.cn))#", std::regex_constants::icase);
    const std::regex AURORA_CHINA_CLUSTER_PATTERN(
        R"#((.+)\.(cluster-|cluster-ro-)+([a-zA-Z0-9]+\.(rds\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+\.rds)\.amazonaws\.com\.cn))#",
        std::regex_constants::icase);
    const std::regex AURORA_CHINA_WRITER_CLUSTER_PATTERN(
        R"#((.+)\.(cluster-)+([a-zA-Z0-9]+\.(rds\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+\.rds)\.amazonaws\.com\.cn))#", std::regex_constants::icase);
    const std::regex AURORA_CHINA_READER_CLUSTER_PATTERN(
        R"#((.+)\.(cluster-ro-)+([a-zA-Z0-9]+\.(rds\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+\.rds)\.amazonaws\.com\.cn))#", std::regex_constants::icase);
    const std::regex AURORA_CHINA_CUSTOM_CLUSTER_PATTERN(
        R"#((.+)\.(cluster-custom-)+([a-zA-Z0-9]+\.(rds\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+\.rds)\.amazonaws\.com\.cn))#", std::regex_constants::icase);
    const std::regex IPV4_PATTERN(
        R"#(^(([1-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){1}(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){2}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$)#");
    const std::regex IPV6_PATTERN(R"#(^[0-9a-fA-F]{1,4}(:[0-9a-fA-F]{1,4}){7}$)#");
    const std::regex IPV6_COMPRESSED_PATTERN(R"#(^(([0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4}){0,5})?)::(([0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4}){0,5})?)$)#");
}

bool RdsUtils::is_dns_pattern_valid(const std::string& host) {
    return ( host.find('?') != std::string::npos);
}

bool RdsUtils::is_rds_dns(const std::string& host) {
    return std::regex_match(host, AURORA_DNS_PATTERN) || std::regex_match(host, AURORA_CHINA_DNS_PATTERN);
}

bool RdsUtils::is_rds_cluster_dns(const std::string& host) {
    return std::regex_match(host, AURORA_CLUSTER_PATTERN) || std::regex_match(host, AURORA_CHINA_CLUSTER_PATTERN);
}

bool RdsUtils::is_rds_proxy_dns(const std::string& host) {
    return std::regex_match(host, AURORA_PROXY_DNS_PATTERN) || std::regex_match(host, AURORA_CHINA_PROXY_DNS_PATTERN);
}

bool RdsUtils::is_rds_writer_cluster_dns(const std::string& host) {
    return std::regex_match(host, AURORA_WRITER_CLUSTER_PATTERN) || std::regex_match(host, AURORA_CHINA_WRITER_CLUSTER_PATTERN);
}

bool RdsUtils::is_rds_reader_cluster_dns(const std::string& host) {
    return std::regex_match(host, AURORA_READER_CLUSTER_PATTERN) || std::regex_match(host, AURORA_CHINA_READER_CLUSTER_PATTERN);
}

bool RdsUtils::is_rds_custom_cluster_dns(const std::string& host) {
    return std::regex_match(host, AURORA_CUSTOM_CLUSTER_PATTERN) || std::regex_match(host, AURORA_CHINA_CUSTOM_CLUSTER_PATTERN);
}

std::string RdsUtils::get_rds_cluster_host_url(const std::string& host) {
    auto f = [ host ](const std::regex& pattern) {
        std::smatch m;
        if (std::regex_search(host, m, pattern) && m.size() > 1) {
            std::string gr1 = m.size() > 1 ? m.str(1) : std::string("");
            std::string gr2 = m.size() > 2 ? m.str(2) : std::string("");
            std::string gr3 = m.size() > 3 ? m.str(3) : std::string("");
            if (!gr1.empty() && !gr3.empty() &&
                (strcmp_case_insensitive(gr2.c_str(), "cluster-") == 0 || strcmp_case_insensitive(gr2.c_str(), "cluster-ro-") == 0)) {
                std::string result;
                result.assign(gr1);
                result.append(".cluster-");
                result.append(gr3);

                return result;
            }
        }
        return std::string();
    };

    auto result = f(AURORA_CLUSTER_PATTERN);
    if (!result.empty()) {
        return result;
    }

    return f(AURORA_CHINA_CLUSTER_PATTERN);
}

std::string RdsUtils::get_rds_cluster_id(const std::string& host) {
    auto f = [ host ](const std::regex& pattern) {
        std::smatch m;
        if (std::regex_search(host, m, pattern) && m.size() > 1 && !m.str(2).empty()) {
            return m.str(1);
        }
        return std::string();
    };

    auto result = f(AURORA_DNS_PATTERN);
    if (!result.empty()) {
        return result;
    }

    return f(AURORA_CHINA_DNS_PATTERN);
}

std::string RdsUtils::get_rds_instance_id(const std::string& host) {
    auto f = [ host ](const std::regex& pattern) {
        std::smatch m;
        if (std::regex_search(host, m, pattern) && m.size() > 1 && m.str(2).empty()) {
            return m.str(1);
        }
        return std::string();
    };

    auto result = f(AURORA_DNS_PATTERN);
    if (!result.empty()) {
        return result;
    }

    return f(AURORA_CHINA_DNS_PATTERN);
}

std::string RdsUtils::get_rds_instance_host_pattern(const std::string& host) {
    auto f = [ host ](const std::regex& pattern) {
        std::smatch m;
        if (std::regex_search(host, m, pattern) && m.size() > 4 && !m.str(3).empty()) {
            std::string result("?.");
            result.append(m.str(3));

            return result;
        }
        return std::string();
    };

    auto result = f(AURORA_DNS_PATTERN);
    if (!result.empty()) {
        return result;
    }

    return f(AURORA_CHINA_DNS_PATTERN);
}

std::string RdsUtils::get_rds_region(const std::string& host) {
    auto f = [ host ](const std::regex& pattern) {
        std::smatch m;
        if (std::regex_search(host, m, pattern) && m.size() > 4 && !m.str(4).empty()) {
            return m.str(4);
        }
        return std::string();
    };

    auto result = f(AURORA_DNS_PATTERN);
    if (!result.empty()) {
        return result;
    }

    return f(AURORA_CHINA_DNS_PATTERN);
}

bool RdsUtils::is_ipv4(const std::string& host) {
    return std::regex_match(host, IPV4_PATTERN);
}

bool RdsUtils::is_ipv6(const std::string& host) {
    return std::regex_match(host, IPV6_PATTERN) || std::regex_match(host, IPV6_COMPRESSED_PATTERN);
}

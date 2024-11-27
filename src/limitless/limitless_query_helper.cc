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

#include "limitless_query_helper.h"

#include <cmath>
#include <string>

#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"

const std::string LimitlessQueryHelper::LIMITLESS_ROUTER_ENDPOINT_QUERY  =
      "SELECT router_endpoint, load FROM aurora_limitless_router_endpoints()";

std::vector<HostInfo> LimitlessQueryHelper::QueryForLimitlessRouters(SQLHDBC conn, int host_port_to_map) {
    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", hstmt, SQL_HANDLE_STMT)) {
        return std::vector<HostInfo>();
    }

    // Generally accepted URL endpoint max length + 1 for null terminator
    SQLCHAR router_endpoint_value[ROUTER_ENDPOINT_LENGTH] = {0};
    SQLLEN ind_router_endpoint_value = 0;

    SQLCHAR load_value[LOAD_LENGTH] = {0};
    SQLLEN ind_load_value = 0;

    rc = SQLBindCol(hstmt, 1, SQL_C_CHAR, &router_endpoint_value, sizeof(router_endpoint_value), &ind_router_endpoint_value);
    SQLRETURN rc2 = SQLBindCol(hstmt, 2, SQL_C_CHAR, &load_value, sizeof(load_value), &ind_load_value);
    if (!OdbcHelper::CheckResult(rc, "SQLBindCol for router endpoint failed", hstmt, SQL_HANDLE_STMT) || 
        !OdbcHelper::CheckResult(rc2, "SQLBindCol for load value failed", hstmt, SQL_HANDLE_STMT)) {
        return std::vector<HostInfo>();
    }

    rc = SQLExecDirect(hstmt, const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(LimitlessQueryHelper::LIMITLESS_ROUTER_ENDPOINT_QUERY.c_str())), SQL_NTS);
    if (!OdbcHelper::CheckResult(rc, "SQLExecDirect failed", hstmt, SQL_HANDLE_STMT)) {
        return std::vector<HostInfo>();
    }

    SQLLEN row_count = 0;
    rc = SQLRowCount(hstmt, &row_count);
    if (!OdbcHelper::CheckResult(rc, "SQLRowCount failed", hstmt, SQL_HANDLE_STMT)) {
        return std::vector<HostInfo>();
    }
    std::vector<HostInfo> limitless_routers;

    while (SQL_SUCCEEDED(rc = SQLFetch(hstmt))) {
        limitless_routers.push_back(CreateHost(load_value, router_endpoint_value, host_port_to_map));
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return limitless_routers;
}

HostInfo LimitlessQueryHelper::CreateHost(const SQLCHAR* load, const SQLCHAR* router_endpoint, const int host_port_to_map) {
    int64_t weight = std::round(WEIGHT_SCALING - (atof(reinterpret_cast<const char *>(load)) * WEIGHT_SCALING));

    if (weight < MIN_WEIGHT || weight > MAX_WEIGHT) {
        weight = MIN_WEIGHT;
        LOG(WARNING) << "Invalid router load of " << load << " for " << router_endpoint;
    }

    std::string router_endpoint_str(reinterpret_cast<const char *>(router_endpoint));

    return HostInfo(
            router_endpoint_str,
            host_port_to_map,
            UP,
            true,
            nullptr,
            weight
        );
}

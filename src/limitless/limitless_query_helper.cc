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

#include "limitless_query_helper.h"

#include <sql.h>
#include <sqlext.h>

#include <cmath>
#include <string>

#include <glog/logging.h>

#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"
#include "../util/string_to_number_converter.h"

SQLTCHAR* LimitlessQueryHelper::check_limitless_cluster_query = AS_SQLTCHAR(TEXT(\
    "SELECT EXISTS ("\
    "   SELECT 1"\
    "   FROM pg_catalog.pg_class c"\
    "   JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid"\
    "   WHERE c.relname = 'limitless_subclusters'"\
    "   AND n.nspname = 'rds_aurora'"\
    ");"\
));

SQLTCHAR* LimitlessQueryHelper::limitless_router_endpoint_query = \
    AS_SQLTCHAR(TEXT("SELECT router_endpoint, load FROM aurora_limitless_router_endpoints()"));

bool LimitlessQueryHelper::CheckLimitlessCluster(SQLHDBC conn) {
    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        return false;
    }

    rc = SQLExecDirect(hstmt, check_limitless_cluster_query, SQL_NTS);
    if (!OdbcHelper::CheckResult(rc, "CheckLimitlessCluster - SQLExecDirect failed", hstmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return false;
    }

    rc = SQLFetch(hstmt);
    if (!OdbcHelper::CheckResult(rc, "CheckLimitlessCluster - SQLFetch failed", hstmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return false;
    }

    SQLCHAR result[2];
    SQLLEN result_len = 0;
    rc = SQLGetData(hstmt, 1, SQL_C_CHAR, &result, sizeof(result), &result_len);
    if (OdbcHelper::CheckResult(rc, "CheckLimitlessCluster - SQLGetData failed", hstmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return result[0] == '1';
    }

    OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
    return false;
}

std::vector<HostInfo> LimitlessQueryHelper::QueryForLimitlessRouters(SQLHDBC conn, int host_port_to_map) {
    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!OdbcHelper::CheckResult(rc, "LimitlessQueryHelper: SQLAllocHandle failed", hstmt, SQL_HANDLE_STMT)) {
        return std::vector<HostInfo>();
    }

    // Generally accepted URL endpoint max length + 1 for null terminator
    SQLCHAR router_endpoint_value[ROUTER_ENDPOINT_LENGTH] = {0};
    SQLLEN ind_router_endpoint_value = 0;

    SQLCHAR load_value[LOAD_LENGTH] = {0};
    SQLLEN ind_load_value = 0;

    rc = SQLBindCol(hstmt, 1, SQL_C_CHAR, &router_endpoint_value, sizeof(router_endpoint_value), &ind_router_endpoint_value);
    SQLRETURN rc2 = SQLBindCol(hstmt, 2, SQL_C_CHAR, &load_value, sizeof(load_value), &ind_load_value);
    if (!OdbcHelper::CheckResult(rc, "LimitlessQueryHelper: SQLBindCol for router endpoint failed", hstmt, SQL_HANDLE_STMT) ||
        !OdbcHelper::CheckResult(rc2, "LimitlessQueryHelper: SQLBindCol for load value failed", hstmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return std::vector<HostInfo>();
    }

    rc = SQLExecDirect(hstmt, limitless_router_endpoint_query, SQL_NTS);
    if (!OdbcHelper::CheckResult(rc, "LimitlessQueryHelper: SQLExecDirect failed", hstmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return std::vector<HostInfo>();
    }

    SQLLEN row_count = 0;
    rc = SQLRowCount(hstmt, &row_count);
    if (!OdbcHelper::CheckResult(rc, "LimitlessQueryHelper: SQLRowCount failed", hstmt, SQL_HANDLE_STMT)) {
        OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return std::vector<HostInfo>();
    }
    std::vector<HostInfo> limitless_routers;

    while (SQL_SUCCEEDED(rc = SQLFetch(hstmt))) {
        limitless_routers.push_back(create_host(load_value, router_endpoint_value, host_port_to_map));
    }

    OdbcHelper::Cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);

    return limitless_routers;
}

HostInfo LimitlessQueryHelper::create_host(const SQLCHAR* load, const SQLCHAR* router_endpoint, const int host_port_to_map) {
    int64_t weight = std::round(WEIGHT_SCALING - (StringToNumberConverter::toDouble(reinterpret_cast<const char *>(load)) * WEIGHT_SCALING));

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

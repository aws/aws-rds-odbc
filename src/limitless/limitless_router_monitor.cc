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

#include "limitless_router_monitor.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <sqlext.h>

#include <chrono>
#include <regex>

#include "../util/connection_string_helper.h"
#include "../util/connection_string_keys.h"
#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"
#include "limitless_query_helper.h"

LimitlessRouterMonitor::LimitlessRouterMonitor() = default;

LimitlessRouterMonitor::~LimitlessRouterMonitor() {
    this->Close();
    this->limitless_routers = nullptr;
    this->limitless_routers_mutex = nullptr;
}

void LimitlessRouterMonitor::Open(
    bool block_and_query_immediately,
    const SQLTCHAR *connection_string_c_str,
    int host_port,
    unsigned int interval_ms,
    std::shared_ptr<std::vector<HostInfo>>& limitless_routers,
    std::shared_ptr<std::mutex>& limitless_routers_mutex
) {
    this->interval_ms = interval_ms;
    this->limitless_routers = limitless_routers;
    this->limitless_routers_mutex = limitless_routers_mutex;

    SQLHENV henv = SQL_NULL_HANDLE;
    SQLHDBC conn = SQL_NULL_HANDLE;

    this->connection_string = StringHelper::ToSQLSTR(connection_string_c_str);

    // disable limitless for the monitor
    RDSREGEX limitless_enabled_pattern(LIMITLESS_ENABLED_KEY TEXT("=") BOOL_TRUE);
    SQLSTR limitless_disabled = LIMITLESS_ENABLED_KEY TEXT("=") BOOL_FALSE;
    this->connection_string = std::regex_replace(this->connection_string, limitless_enabled_pattern, limitless_disabled);

    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
    if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLAllocHandle failed", henv, SQL_HANDLE_ENV)) {
        return; // fatal error; don't open thread
    }

    SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);

    if (block_and_query_immediately) {
        rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
        if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLAllocHandle failed", conn, SQL_HANDLE_DBC)) {
            OdbcHelper::Cleanup(henv, conn, SQL_NULL_HSTMT);
            return; // fatal error; don't open thread
        }

        rc = SQLDriverConnect(conn, nullptr, AS_SQLTCHAR(this->connection_string.c_str()), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        if (SQL_SUCCEEDED(rc)) {
            // initial connection was successful, immediately populate caller's limitless routers
            *limitless_routers = LimitlessQueryHelper::QueryForLimitlessRouters(conn, host_port);
        } else {
            // not successful, ensure limitless routers is empty 
            limitless_routers->clear();
        }
    }

    // start monitoring thread; if block_and_query_immediately is false, then conn is SQL_NULL_HANDLE, and the thread will connect after the monitor interval has passed
    this->monitor_thread = std::make_shared<std::thread>(&LimitlessRouterMonitor::Run, this, henv, conn, reinterpret_cast<SQLTCHAR *>(this->connection_string.data()), SQL_NTS, host_port);
}

bool LimitlessRouterMonitor::TestConnectionToHost(const std::string &server) {
    SQLHENV henv = nullptr;
    SQLHDBC conn = nullptr;

    // build new connection string, overwriting server to provided server
    std::map<SQLSTR, SQLSTR> connstr_map;
    ConnectionStringHelper::ParseConnectionString(this->connection_string, connstr_map);
    connstr_map[SERVER_HOST_KEY] = StringHelper::ToSQLSTR(server);
    SQLSTR connstr = ConnectionStringHelper::BuildConnectionString(connstr_map);

    // allocate a new henv for the test connection
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
    if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLAllocHandle failed in test connection", henv, SQL_HANDLE_ENV)) {
        return false;
    }

    // set odbc version
    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLSetEnvAttr failed in test connection", henv, SQL_HANDLE_ENV)) {
        return false;
    }

    // allocate connection handle
    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
    if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLAllocHandle failed in test connection", conn, SQL_HANDLE_DBC)) {
        OdbcHelper::Cleanup(henv, conn, SQL_NULL_HSTMT);
        return false;
    }

    // attempt to connect
    rc = SQLDriverConnect(conn, nullptr, AS_SQLTCHAR(connstr.c_str()), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLDriverConnect failed in test connection", conn, SQL_HANDLE_DBC)) {
        OdbcHelper::Cleanup(henv, conn, SQL_NULL_HSTMT);
        return false;
    }

    // check the connection with a simple query - if successful, connection is OK
    bool test_query_successful = OdbcHelper::CheckConnection(conn);
    OdbcHelper::Cleanup(henv, conn, SQL_NULL_HSTMT); // disconnects conn
    return test_query_successful;
}

bool LimitlessRouterMonitor::IsStopped() {
    return this->stopped;
}

void LimitlessRouterMonitor::Close() {
    if (this->stopped) {
        return;
    }

    this->stopped = true;

    if (this->monitor_thread != nullptr) {
        this->monitor_thread->join();
        this->monitor_thread = nullptr;
    }
}

void LimitlessRouterMonitor::Run(SQLHENV henv, SQLHDBC conn, SQLTCHAR *connection_string, SQLSMALLINT connection_string_len, int host_port) {
    SQLSMALLINT out_connection_string_len; // unused

    while (!this->stopped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_ms));

        if (conn == SQL_NULL_HANDLE || !OdbcHelper::CheckConnection(conn)) {
            // OdbcHelper::CheckConnection failed on a pre-existing handle, so free it
            OdbcHelper::Cleanup(SQL_NULL_HENV, conn, SQL_NULL_HSTMT);

            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
            if (!OdbcHelper::CheckResult(rc, "LimitlessRouterMonitor: SQLAllocHandle failed", conn, SQL_HANDLE_DBC)) {
                break; // this is a fatal error; stop monitoring
            }

            rc = SQLDriverConnect(conn, nullptr, connection_string, connection_string_len, nullptr, 0, &out_connection_string_len, SQL_DRIVER_NOPROMPT);
            if (!SQL_SUCCEEDED(rc)) {
                OdbcHelper::Cleanup(SQL_NULL_HENV, conn, SQL_NULL_HSTMT);

                // wait the full interval and then try to reconnect
                continue;
            } // else, connection was successful, proceed below
        }

        std::vector<HostInfo> new_limitless_routers = LimitlessQueryHelper::QueryForLimitlessRouters(conn, host_port);

        // LimitlessQueryHelper::QueryForLimitlessRouters will return an empty vector on an error
        // if it was a connection error, then the next loop will catch it and attempt to reconnect
        if (!new_limitless_routers.empty()) {
            std::lock_guard<std::mutex> guard(*(this->limitless_routers_mutex));
            *(this->limitless_routers) = new_limitless_routers;
        }
    }

    OdbcHelper::Cleanup(henv, conn, SQL_NULL_HSTMT);
}

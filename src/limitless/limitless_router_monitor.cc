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

#include <chrono>
#include <cstring>

#include "../util/logger_wrapper.h"
#include "../util/odbc_helper.h"
#include "limitless_query_helper.h"
#include "limitless_router_monitor.h"

LimitlessRouterMonitor::LimitlessRouterMonitor() = default;

LimitlessRouterMonitor::~LimitlessRouterMonitor() {
    this->LimitlessRouterMonitor::Close();
    this->limitless_routers = nullptr;
    this->limitless_routers_mutex = nullptr;
}

void LimitlessRouterMonitor::Open(
    const char *connection_string_c_str,
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
    auto *connection_string = const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(connection_string_c_str));
    SQLSMALLINT connection_string_len = strlen(connection_string_c_str);
    SQLSMALLINT out_connection_string_len; // unused

    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
    if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", henv, SQL_HANDLE_ENV)) {
        return; // fatal error; don't open thread
    }

    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
    if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", conn, SQL_HANDLE_DBC)) {
        return; // fatal error; don't open thread
    }

    rc = SQLDriverConnect(conn, nullptr, connection_string, connection_string_len, nullptr, 0, &out_connection_string_len, SQL_DRIVER_NOPROMPT);
    if (SQL_SUCCEEDED(rc)) {
        // initial connection was successful, immediately populate caller's limitless routers
        *limitless_routers = LimitlessQueryHelper::QueryForLimitlessRouters(conn, host_port);
    } else {
        // not successful, ensure limitless routers is empty 
        limitless_routers->clear();
    }

    // start monitoring thread
    this->monitor_thread = std::make_shared<std::thread>(&LimitlessRouterMonitor::run, this, henv, conn, connection_string, connection_string_len, host_port);
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

void LimitlessRouterMonitor::run(SQLHENV henv, SQLHDBC conn, SQLCHAR *connection_string, SQLSMALLINT connection_string_len, int host_port) {
    SQLSMALLINT out_connection_string_len; // unused

    while (!this->stopped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_ms));

        if (conn == SQL_NULL_HANDLE || !OdbcHelper::CheckConnection(conn)) {
            // OdbcHelper::CheckConnection failed on a pre-existing handle, so free it
            if (conn != SQL_NULL_HANDLE) {
                SQLFreeHandle(SQL_HANDLE_DBC, conn);
                conn = SQL_NULL_HANDLE;
            }

            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
            if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", conn, SQL_HANDLE_DBC)) {
                break; // this is a fatal error; stop monitoring
            }

            rc = SQLDriverConnect(conn, nullptr, connection_string, connection_string_len, nullptr, 0, &out_connection_string_len, SQL_DRIVER_NOPROMPT);
            if (!SQL_SUCCEEDED(rc)) {
                SQLFreeHandle(SQL_HANDLE_DBC, conn);
                conn = SQL_NULL_HANDLE; // next loop can re-attempt connection

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

    if (conn != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_DBC, conn);
        conn = SQL_NULL_HANDLE;
    }

    SQLFreeHandle(SQL_HANDLE_ENV, henv);
    henv = SQL_NULL_HANDLE;
}

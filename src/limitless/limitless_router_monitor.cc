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

LimitlessRouterMonitor::LimitlessRouterMonitor(
    const char *connection_string,
    int host_port,
    unsigned int interval_ms,
    std::shared_ptr<std::vector<HostInfo>> limitless_routers
) :
    interval_ms(interval_ms),
    limitless_routers(std::move(limitless_routers)),
    monitor_thread(std::make_shared<std::thread>(&LimitlessRouterMonitor::run, this, connection_string, host_port))
{
}

LimitlessRouterMonitor::~LimitlessRouterMonitor() {
    this->Close();
}

bool LimitlessRouterMonitor::IsStopped() {
    return this->stopped;
}

void LimitlessRouterMonitor::Close() {
    if (this->stopped) {
        return;
    }

    this->stopped = true;
    this->monitor_thread->join();
}

void LimitlessRouterMonitor::run(const char *connection_string, int host_port) {
    SQLHENV henv = SQL_NULL_HANDLE;
    SQLHDBC conn = SQL_NULL_HANDLE;
    SQLSMALLINT connection_string_len = strlen(connection_string);
    SQLSMALLINT out_connection_string_len; // unused

    SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
    SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
    SQLDriverConnect(
        conn,
        nullptr,
        const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(connection_string)),
        connection_string_len,
        nullptr, // connection_string should be complete
        0,
        &out_connection_string_len,
        SQL_DRIVER_NOPROMPT
    );

    while (!this->stopped) {
        if (conn != SQL_NULL_HANDLE && !OdbcHelper::CheckConnection(conn)) {
            SQLFreeHandle(SQL_HANDLE_DBC, conn);
            conn = SQL_NULL_HANDLE;
        }
        
        if (conn == SQL_NULL_HANDLE) {
            SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn);
            if (!OdbcHelper::CheckResult(rc, "SQLAllocHandle failed", conn, SQL_HANDLE_DBC)) {
                break; // this is a fatal error
            }

            rc = SQLDriverConnect(
                conn,
                nullptr,
                const_cast<SQLCHAR *>(reinterpret_cast<const SQLCHAR *>(connection_string)),
                connection_string_len,
                nullptr, // connection_string should be complete
                0,
                &out_connection_string_len,
                SQL_DRIVER_NOPROMPT
            );
            if (!SQL_SUCCEEDED(rc)) {
                SQLFreeHandle(SQL_HANDLE_DBC, conn);
                conn = SQL_NULL_HANDLE;

                // wait the full interval and then try to reconnect
                std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_ms));
                continue;
            }

            // connection was successful; query for limitless routers
        }

        std::vector<HostInfo> new_limitless_routers = LimitlessQueryHelper::QueryForLimitlessRouters(conn, host_port);

        // LimitlessQueryHelper::QueryForLimitlessRouters will return an empty vector on an error
        // if it was a connection error, then the next loop will catch it and attempt to reconnect
        if (!new_limitless_routers.empty()) {
            *(this->limitless_routers) = new_limitless_routers;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_ms));
    }

    if (conn != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_DBC, conn);
        conn = SQL_NULL_HANDLE;
    }

    SQLFreeHandle(SQL_HANDLE_ENV, henv);
    henv = SQL_NULL_HANDLE;
}

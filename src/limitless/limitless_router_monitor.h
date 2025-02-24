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

#ifndef LIMITLESSROUTERMONITOR_H_
#define LIMITLESSROUTERMONITOR_H_

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#ifdef WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include "../host_info.h"

class LimitlessRouterMonitor {
public:
    LimitlessRouterMonitor();

    ~LimitlessRouterMonitor();

    void Close();

    virtual void Open(
        bool block_and_query_immediately,
        const SQLTCHAR *connection_string_c_str,
        int host_port,
        unsigned int interval_ms,
        std::shared_ptr<std::vector<HostInfo>>& limitless_routers,
        std::shared_ptr<std::mutex>& limitless_routers_mutex
    );

    virtual bool IsStopped();
protected:
#ifdef UNICODE
    std::wstring connection_string;
#else
    std::string connection_string;
#endif
    
    std::atomic_bool stopped = false;

    unsigned int interval_ms;

    std::shared_ptr<std::vector<HostInfo>> limitless_routers;

    std::shared_ptr<std::mutex> limitless_routers_mutex;

    std::shared_ptr<std::thread> monitor_thread = nullptr;

    void run(SQLHENV henv, SQLHDBC conn, SQLTCHAR *connection_string, SQLSMALLINT connection_string_len, int host_port);
};

#ifdef UNICODE
#define LIMITLESS_ENABLED_KEY   L"LIMITLESSENABLED"
#else
#define LIMITLESS_ENABLED_KEY   "LIMITLESSENABLED"
#endif

#endif // LIMITLESSROUTERMONITOR_H_

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

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

#ifndef __MOCKOBJECTS_H__
#define __MOCKOBJECTS_H__

#include <mutex>
#include <thread>

#include <gmock/gmock.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>

#include "authentication_provider.h"
#include "cluster_topology_monitor.h"
#include "cluster_topology_query_helper.h"
#include "federation.h"
#include "limitless_monitor_service.h"

#include "util/odbc_helper.h"

#define TEST_LIMITLESS_MONITOR_INTERVAL_MS 250

class MOCK_SECRETS_MANAGER_CLIENT : public Aws::SecretsManager::SecretsManagerClient {
public:
    MOCK_SECRETS_MANAGER_CLIENT() : SecretsManagerClient() {};

    MOCK_METHOD(Aws::SecretsManager::Model::GetSecretValueOutcome, GetSecretValue, (const Aws::SecretsManager::Model::GetSecretValueRequest&), (const));
};

class MOCK_HTTP_RESP : public Aws::Http::HttpResponse {
public:
    MOCK_HTTP_RESP() : Aws::Http::HttpResponse(nullptr){}
    ~MOCK_HTTP_RESP() override {}

    MOCK_METHOD(Aws::Http::HttpResponseCode, GetResponseCode, (), (const));
    MOCK_METHOD(std::shared_ptr<Aws::Http::HttpResponse>, MakeRequest, (
        const std::shared_ptr<Aws::Http::HttpRequest>&), (const));
    MOCK_METHOD(Aws::IOStream&, GetResponseBody, (), (const));
    MOCK_METHOD(bool, HasClientError, (), (const));
    MOCK_METHOD(Aws::String&, GetClientErrorMessage, (), (const));

    // Not used in test, required to instantiate abstract class
    MOCK_METHOD(Aws::Http::HttpRequest&, GetOriginatingRequest, (), (const));
    MOCK_METHOD(void, SetOriginatingRequest, (const std::shared_ptr<const Aws::Http::HttpRequest>&), ());
    MOCK_METHOD(Aws::Http::HeaderValueCollection, GetHeaders, (), (const));
    MOCK_METHOD(bool, HasHeader, (const char* headerName), (const));
    MOCK_METHOD(Aws::String&, GetHeader, (const Aws::String& headerName), (const));
    MOCK_METHOD(void, SetResponseCode, (Aws::Http::HttpResponseCode httpResponseCode), (const));
    MOCK_METHOD(Aws::String&, GetContentType, (), (const));
    MOCK_METHOD(Aws::Utils::Stream::ResponseStream&&, SwapResponseStreamOwnership, (), ());
    MOCK_METHOD(void, AddHeader, (const Aws::String&, const Aws::String&), ());
    MOCK_METHOD(void, AddHeader, (const Aws::String&, const Aws::String&&), ());
    MOCK_METHOD(void, SetContentType, (const Aws::String&), ());
};

class MOCK_HTTP_CLIENT : public Aws::Http::HttpClient {
public:
    MOCK_METHOD(std::shared_ptr<Aws::Http::HttpResponse>, MakeRequest, (
        const std::shared_ptr<Aws::Http::HttpRequest>&), (const));

    // Not used in test, required to instantiate abstract class
    MOCK_METHOD(std::shared_ptr<Aws::Http::HttpResponse>, MakeRequest, (
        const std::shared_ptr<Aws::Http::HttpRequest>&,
        Aws::Utils::RateLimits::RateLimiterInterface*,
        Aws::Utils::RateLimits::RateLimiterInterface*), (const));
    MOCK_METHOD(bool, SupportsChunkedTransferEncoding, (), (const));
};

class MOCK_STS_CLIENT : public Aws::STS::STSClient {
public:
    MOCK_METHOD(Aws::STS::Model::AssumeRoleWithSAMLOutcome, AssumeRoleWithSAML, (const Aws::STS::Model::AssumeRoleWithSAMLRequest&), (const));
    MOCK_METHOD(bool, SupportsChunkedTransferEncoding, (), (const));
};

class MOCK_LIMITLESS_ROUTER_MONITOR : public LimitlessRouterMonitor {
public:
    MOCK_METHOD(void, Open, (bool, const SQLTCHAR *, int, unsigned int, std::shared_ptr<std::vector<HostInfo>>&, std::shared_ptr<std::mutex>&), ());
    MOCK_METHOD(bool, IsStopped, (), ());

    std::vector<HostInfo> test_limitless_routers;

    void mock_run(std::shared_ptr<std::vector<HostInfo>> limitless_routers, std::shared_ptr<std::mutex> limitless_routers_mutex) {
        std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_ms));
        std::lock_guard<std::mutex> guard(*limitless_routers_mutex);
        *limitless_routers = this->test_limitless_routers;
        // join
    }

    void MockOpen(
        bool block_and_query_immediately,
        const SQLTCHAR *conn_str, // unused
        int port, // unused
        unsigned int interval_ms,
        std::shared_ptr<std::vector<HostInfo>>& limitless_routers,
        std::shared_ptr<std::mutex>& limitless_routers_mutex
    ) {
        this->interval_ms = TEST_LIMITLESS_MONITOR_INTERVAL_MS;

        if (block_and_query_immediately) {
            std::lock_guard<std::mutex> guard(*limitless_routers_mutex);
            *limitless_routers = this->test_limitless_routers;
        } else {
            this->monitor_thread = std::make_shared<std::thread>(&MOCK_LIMITLESS_ROUTER_MONITOR::mock_run, this, limitless_routers, limitless_routers_mutex);
        }
    }
};

class MOCK_ODBC_HELPER : public IOdbcHelper {
public:
    MOCK_METHOD(bool, ConnStrConnect, (SQLTCHAR*, SQLHDBC&), ());
    MOCK_METHOD(bool, CheckResult, (SQLRETURN, const std::string&, SQLHANDLE, int32_t), ());
    MOCK_METHOD(bool, CheckConnection, (SQLHDBC), ());
    MOCK_METHOD(void, Cleanup, (SQLHENV, SQLHDBC, SQLHSTMT), ());
    MOCK_METHOD(bool, AllocateHandle, (SQLSMALLINT, SQLHANDLE, SQLHANDLE&, const std::string&), ());
    MOCK_METHOD(bool, SetHenvToOdbc3, (SQLHENV, const std::string&), ());
    MOCK_METHOD(bool, ExecuteQuery, (SQLHSTMT, SQLTCHAR*, const std::string&), ());
    MOCK_METHOD(bool, BindColumn, (SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, const std::string&), ());
    MOCK_METHOD(bool, FetchResults, (SQLHSTMT, const std::string&), ());
    MOCK_METHOD(std::string, MergeDiagRecs, (SQLHANDLE handle, int32_t handle_type, const std::string &custom_errmsg), ());
    MOCK_METHOD(bool, TestConnectionToServer, (const SQLSTR &in_conn_str, const std::string &server), ());
};

class MOCK_CLUSTER_TOPOLOGY_QUERY_HELPER : public ClusterTopologyQueryHelper {
public:
    MOCK_CLUSTER_TOPOLOGY_QUERY_HELPER() : ClusterTopologyQueryHelper(0, "", TEXT(""), TEXT(""), TEXT("")) {}
    MOCK_METHOD(std::string, GetWriterId, (SQLHDBC), ());
    MOCK_METHOD(std::string, GetNodeId, (SQLHDBC), ());
    MOCK_METHOD(std::vector<HostInfo>, QueryTopology, (SQLHDBC), ());
    MOCK_METHOD(HostInfo, CreateHost, (SQLHDBC), ());
};

class MOCK_TOPOLOGY_MONITOR : public ClusterTopologyMonitor {
public:
    MOCK_TOPOLOGY_MONITOR(std::shared_ptr<IOdbcHelper> odbc_helper)
        : ClusterTopologyMonitor("", nullptr, AS_SQLTCHAR(TEXT("")), odbc_helper, nullptr, 0, 0, 0) {}
    MOCK_METHOD(void, SetClusterId, (const std::string&), ());
    MOCK_METHOD(std::vector<HostInfo>, ForceRefresh, (bool, uint32_t), ());
    MOCK_METHOD(std::vector<HostInfo>, ForceRefresh, (SQLHDBC, uint32_t), ());
    MOCK_METHOD(void, StartMonitor, (), ());
};

#endif /* __MOCKOBJECTS_H__ */

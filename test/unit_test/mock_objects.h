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
#include "cluster_topology_query_helper.h"
#include "federation.h"
#include "limitless_monitor_service.h"

#include "util/odbc_helper.h"

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
        bool block_and_query_immediately, // unused
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
    MOCK_METHOD(bool, CheckResult, (SQLRETURN, const std::string&, SQLHANDLE, int32_t), ());
    MOCK_METHOD(bool, CheckConnection, (SQLHDBC), ());
    MOCK_METHOD(void, Cleanup, (SQLHENV, SQLHDBC, SQLHSTMT), ());
};

class MOCK_CLUSTER_TOPOLOGY_QUERY_HELPER : public ClusterTopologyQueryHelper {
public:
    MOCK_CLUSTER_TOPOLOGY_QUERY_HELPER() : ClusterTopologyQueryHelper(0, "", "", "", "") {}
    MOCK_METHOD(std::string, get_writer_id, (SQLHDBC hdbc), ());
    MOCK_METHOD(std::string, get_node_id, (SQLHDBC hdbc), ());
    MOCK_METHOD(std::vector<HostInfo>, query_topology, (SQLHDBC hdbc), ());
    MOCK_METHOD(HostInfo, create_host, (SQLHDBC hdbc), ());
};

#endif /* __MOCKOBJECTS_H__ */

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

#ifndef CONNECTION_STRING_KEYS_H
#define CONNECTION_STRING_KEYS_H

#include "text_helper.h"

// Generic
#define SERVER_HOST_KEY TEXT("SERVER")

#define BOOL_FALSE TEXT("0")
#define BOOL_TRUE TEXT("1")

// Failover
#define ENABLE_FAILOVER_KEY TEXT("ENABLECLUSTERFAILOVER")
#define FAILOVER_MODE_KEY TEXT("FAILOVERMODE")
#define READER_HOST_SELECTOR_STRATEGY_KEY TEXT("READERHOSTSELECTORSTRATEGY")
#define ENDPOINT_TEMPLATE_KEY TEXT("HOSTPATTERN")
#define IGNORE_TOPOLOGY_REQUEST_KEY TEXT("IGNORETOPOLOGYREQUEST")
#define HIGH_REFRESH_RATE_KEY TEXT("TOPOLOGYHIGHREFRESHRATE")
#define REFRESH_RATE_KEY TEXT("TOPOLOGYREFRESHRATE")
#define FAILOVER_TIMEOUT_KEY TEXT("FAILOVERTIMEOUT")
#define CLUSTER_ID_KEY TEXT("CLUSTERID")

#endif // CONNECTION_STRING_KEYS_H

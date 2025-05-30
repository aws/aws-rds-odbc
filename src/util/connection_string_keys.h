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

#include "string_helper.h"

// Generic
#define SERVER_HOST_KEY TEXT("SERVER")

#define BOOL_FALSE TEXT("0")
#define BOOL_TRUE TEXT("1")

// Limitless
#define LIMITLESS_ENABLED_KEY TEXT("LIMITLESSENABLED")
#define LIMITLESS_MODE_KEY TEXT("LIMITLESSMODE")
#define LIMITLESS_MODE_VALUE_LAZY TEXT("lazy")
#define LIMITLESS_MODE_VALUE_IMMEDIATE TEXT("immediate")
#define LIMITLESS_MONITOR_INTERVAL_MS_KEY TEXT("LIMITLESSMONITORINTERVALMS")

// Failover
#define ENABLE_FAILOVER_KEY TEXT("ENABLECLUSTERFAILOVER")
#define FAILOVER_MODE_KEY TEXT("FAILOVERMODE")
#define FAILOVER_MODE_VALUE_STRICT_READER TEXT("STRICT_READER")
#define FAILOVER_MODE_VALUE_STRICT_WRITER TEXT("STRICT_WRITER")
#define FAILOVER_MODE_VALUE_READER_OR_WRITER TEXT("READER_OR_WRITER")
#define READER_HOST_SELECTOR_STRATEGY_KEY TEXT("READERHOSTSELECTORSTRATEGY")
#define ENDPOINT_TEMPLATE_KEY TEXT("HOSTPATTERN")
#define IGNORE_TOPOLOGY_REQUEST_KEY TEXT("IGNORETOPOLOGYREQUEST")
#define HIGH_REFRESH_RATE_KEY TEXT("TOPOLOGYHIGHREFRESHRATE")
#define REFRESH_RATE_KEY TEXT("TOPOLOGYREFRESHRATE")
#define FAILOVER_TIMEOUT_KEY TEXT("FAILOVERTIMEOUT")
#define CLUSTER_ID_KEY TEXT("CLUSTERID")

#endif // CONNECTION_STRING_KEYS_H

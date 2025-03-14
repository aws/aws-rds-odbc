// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

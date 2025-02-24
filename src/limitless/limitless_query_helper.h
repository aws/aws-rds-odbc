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

#ifndef LIMITLESSQUERYHELPER_H_
#define LIMITLESSQUERYHELPER_H_

#ifdef WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include <vector>

#include "../host_info.h"

class LimitlessQueryHelper {
public:
    static const int ROUTER_ENDPOINT_LENGTH = 2049;
    static const int LOAD_LENGTH = 5;
    static const int WEIGHT_SCALING = 10;
    static const int MAX_WEIGHT = 10;
    static const int MIN_WEIGHT = 1;
    static SQLTCHAR *check_limitless_cluster_query;
    static SQLTCHAR *limitless_router_endpoint_query;

    static bool CheckLimitlessCluster(SQLHDBC conn);
    static std::vector<HostInfo> QueryForLimitlessRouters(SQLHDBC conn, int host_port_to_map);

private:
    static HostInfo CreateHost(const SQLCHAR* load, const SQLCHAR* router_endpoint, int host_port_to_map);
};

#endif // LIMITLESSQUERYHELPER_H_

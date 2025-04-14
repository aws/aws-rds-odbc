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

#ifndef CONNECTION_STRING_HELPER_H_
#define CONNECTION_STRING_HELPER_H_

#include <map>
#include <sstream>
#include <string>

#include "string_helper.h"

class ConnectionStringHelper {
public:
    /**
     * Parses the input connection string into the destination map as key-value pairs.
     * Returns -1 on error and the number of items in dest_map on success.
     */
    static void ParseConnectionString(const MyStr &connection_string, std::map<MyStr, MyStr> &dest_map);

    /**
     * Builds a connection string with the given map
     */
    static MyStr BuildConnectionString(std::map<MyStr, MyStr> &input_map);
};

#endif

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

class ConnectionStringHelper {
public:
    /**
     * Parses the input connection string into the destination map as key-value pairs.
     * Returns -1 on error and the number of items in dest_map on success.
     */
    static void ParseConnectionString(const char *connection_string, std::map<std::string, std::string> &dest_map);
    static void ParseConnectionStringW(const wchar_t *connection_string, std::map<std::wstring, std::wstring> &dest_map);

    /**
     * Builds a connection string with the given map
     */
    static std::string BuildConnectionString(std::map<std::string, std::string> &input_map);
    static std::wstring BuildConnectionStringW(std::map<std::wstring, std::wstring> &input_map);
};

#endif

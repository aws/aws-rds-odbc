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

#include "connection_string_helper.h"

#include <regex>

void ConnectionStringHelper::ParseConnectionString(const SQLSTR &connection_string, std::map<SQLSTR, SQLSTR> &dest_map) {
    RDSREGEX pattern(TEXT("([^;=]+)=([^;]+)"));
    RDSSTRMATCH match;
    SQLSTR conn_str = connection_string;

    while (std::regex_search(conn_str, match, pattern)) {
        SQLSTR key = match[1].str();
        std::transform(key.begin(), key.end(), key.begin(), [](RDSCHAR c) {
            return StringHelper::ToUpper(c);
        });
        SQLSTR val = match[2].str();
        dest_map[key] = val;

        conn_str = match.suffix().str();
    }
}

SQLSTR ConnectionStringHelper::BuildConnectionString(std::map<SQLSTR, SQLSTR> &input_map) {
    RDSSTRSTREAM conn_stream;
    for (const auto& e : input_map) {
        if (conn_stream.tellp() > 0) {
            conn_stream << TEXT(";");
        }
        conn_stream << e.first << TEXT("=") << e.second;
    }
    return conn_stream.str();
}

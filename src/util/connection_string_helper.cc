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

#include <cwctype>
#include <regex>
#include <sstream>

#include "connection_string_helper.h"

void ConnectionStringHelper::ParseConnectionString(const char *connection_string, std::map<std::string, std::string> &dest_map) {
    std::regex pattern("([^;=]+)=([^;]+)");
    std::cmatch match;
    std::string conn_str = connection_string;

    while (std::regex_search(conn_str.c_str(), match, pattern)) {
        std::string key = match[1].str();
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return std::toupper(c);
        });
        std::string val = match[2].str();
        dest_map[key] = val;

        conn_str = match.suffix().str();
    }
}

void ConnectionStringHelper::ParseConnectionStringW(const wchar_t *connection_string, std::map<std::wstring, std::wstring> &dest_map) {
    std::wregex pattern(L"([^;=]+)=([^;]+)");
    std::wsmatch match;
    std::wstring conn_str = connection_string;

    while (std::regex_search(conn_str, match, pattern)) {
        std::wstring key = match[1].str();
        std::transform(key.begin(), key.end(), key.begin(), [](wchar_t c) {
            return std::towupper(c);
        });
        std::wstring val = match[2].str();
        dest_map[key] = val;

        conn_str = match.suffix().str();
    }
}

std::string ConnectionStringHelper::BuildConnectionString(std::map<std::string, std::string> &input_map) {
    std::ostringstream conn_stream;
    for (const auto& e : input_map) {
        if (conn_stream.tellp() > 0) {
            conn_stream << ";";
        }
        conn_stream << e.first << "=" << e.second;
    }
    return conn_stream.str();
}

std::wstring ConnectionStringHelper::BuildConnectionStringW(std::map<std::wstring, std::wstring> &input_map) {
    std::wostringstream conn_stream;
    for (const auto& e : input_map) {
        if (conn_stream.tellp() > 0) {
            conn_stream << ";";
        }
        conn_stream << e.first << "=" << e.second;
    }
    return conn_stream.str();
}

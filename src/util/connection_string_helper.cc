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

#include "connection_string_helper.h"
#include "text_helper.h"

namespace {
    #ifdef UNICODE
    typedef std::wsmatch MyMatch;
    typedef wchar_t MyChar;
    typedef std::wostringstream MyStream;

    #define MyUpper(c) std::towupper(c)
    #else
    typedef std::smatch MyMatch;
    typedef unsigned char MyChar;
    typedef std::ostringstream MyStream;

    #define MyUpper(c) std::toupper(c)
    #endif
};

void ConnectionStringHelper::ParseConnectionString(const MyStr &connection_string, std::map<MyStr, MyStr> &dest_map) {
    MyRegex pattern(TEXT("([^;=]+)=([^;]+)"));
    MyMatch match;
    MyStr conn_str = connection_string;

    while (std::regex_search(conn_str, match, pattern)) {
        MyStr key = match[1].str();
        std::transform(key.begin(), key.end(), key.begin(), [](MyChar c) {
            return MyUpper(c);
        });
        MyStr val = match[2].str();
        dest_map[key] = val;

        conn_str = match.suffix().str();
    }
}

MyStr ConnectionStringHelper::BuildConnectionString(std::map<MyStr, MyStr> &input_map) {
    MyStream conn_stream;
    for (const auto& e : input_map) {
        if (conn_stream.tellp() > 0) {
            conn_stream << ";";
        }
        conn_stream << e.first << "=" << e.second;
    }
    return conn_stream.str();
}

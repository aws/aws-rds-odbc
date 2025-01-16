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

#include <cwctype>
#include <regex>

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
    for (auto e : input_map) {
        if (conn_stream.tellp() > 0) {
            conn_stream << ";";
        }
        conn_stream << e.first << "=" << e.second;
    }
    return conn_stream.str();
}

std::wstring ConnectionStringHelper::BuildConnectionStringW(std::map<std::wstring, std::wstring> &input_map) {
    std::wostringstream conn_stream;
    for (auto e : input_map) {
        if (conn_stream.tellp() > 0) {
            conn_stream << ";";
        }
        conn_stream << e.first << "=" << e.second;
    }
    return conn_stream.str();
}

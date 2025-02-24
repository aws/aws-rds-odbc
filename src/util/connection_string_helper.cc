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

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

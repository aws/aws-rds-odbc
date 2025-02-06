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

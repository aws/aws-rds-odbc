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

#include "connection_string_helper.h"

// taken from aws-pgsql-odbc psqlodbc.h
enum ConnectionStringSizes {
    MAX_CONNECT_STRING = 4096,
    MEDIUM_REGISTRY_LEN = 1024
};

int ConnectionStringHelper::ParseConnectionString(const SQLTCHAR *connection_string, std::map<std::string, std::string> &dest_map) {
    std::string key;
    std::string value;
    SQLTCHAR buffer[MEDIUM_REGISTRY_LEN];
    int j = 0;
    enum { KEY, VALUE } reading_mode = KEY;
    
    for (int i = 0; connection_string[i] != '\0' && i < MAX_CONNECT_STRING; i++) {
        // stop condition for keys are equals
        if (reading_mode == KEY && connection_string[i] == '=') {
            buffer[j] = '\0';
            #ifdef UNICODE
            std::wstring wide_key = reinterpret_cast<wchar_t *>(buffer);
            key = std::string(wide_key.begin(), wide_key.end());
            #else
            key = reinterpret_cast<char *>(buffer);
            #endif
            j = 0;
            reading_mode = VALUE;
            continue; // done with '='
        }

        // stop condition for values are semicolons
        if (reading_mode == VALUE && connection_string[i] == ';') {
            buffer[j] = '\0';
            #ifdef UNICODE
            std::wstring wide_value = reinterpret_cast<wchar_t *>(buffer);
            value = std::string(wide_value.begin(), wide_value.end());
            #else
            value = reinterpret_cast<char *>(buffer);
            #endif
            j = 0;
            reading_mode = KEY;
            dest_map[key] = value;
            continue; // done with ';'
        }

        buffer[j++] = connection_string[i];
        if (j >= MEDIUM_REGISTRY_LEN - 1) {
            return -1; // key/value too long
        }
    }

    // accept a badly terminated value
    if (reading_mode == VALUE) {
        buffer[j] = '\0';
        #ifdef UNICODE
        std::wstring wide_value = reinterpret_cast<wchar_t *>(buffer);
        value = std::string(wide_value.begin(), wide_value.end());
        #else
        value = reinterpret_cast<char *>(buffer);
        #endif
        dest_map[key] = value;
    }

    return dest_map.size();
}

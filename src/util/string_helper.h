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

#ifndef STRING_HELPER_H_
#define STRING_HELPER_H_

#include <codecvt>
#include <locale>

#define AS_SQLTCHAR(str) (const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(str)))
#define AS_CHAR(str) (reinterpret_cast<char*>(str))
#define AS_CONST_CHAR(str) (reinterpret_cast<const char*>(str))
#define AS_WCHAR(str) (reinterpret_cast<wchar_t*>(str))
#define AS_CONST_WCHAR(str) (reinterpret_cast<const wchar_t*>(str))

#if defined(__APPLE__) || defined(__linux__)
#define strcmp_case_insensitive(str1, str2) strcasecmp(str1, str2)
#else
#define strcmp_case_insensitive(str1, str2) strcmpi(str1, str2)
#endif

#ifdef __linux__
typedef std::wstring_convert<std::codecvt_utf8_utf16<char16_t>> converter;
#elifdef __APPLE__
typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#else
typedef std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
#endif

class StringHelper {
   public:
    static std::wstring to_wstring(const std::string& src) {
        if (src.empty()) {
            return std::wstring();
        }
        return converter{}.from_bytes(src);
    }
    static std::string to_string(const std::wstring& src) {
        if (src.empty()) {
            return std::string();
        }
        return converter{}.to_bytes(src);
    }
};

#endif  // STRING_HELPER_H_

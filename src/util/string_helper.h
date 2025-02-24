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

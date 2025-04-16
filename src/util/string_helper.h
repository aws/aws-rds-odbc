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

#ifndef STRING_HELPER_H_
#define STRING_HELPER_H_

#ifdef WIN32
    #include <windows.h>
#endif
#include <sql.h>

#include <string>

#define AS_SQLTCHAR(str) (const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(str)))
#define AS_CHAR(str) (reinterpret_cast<char*>(str))
#define AS_CONST_CHAR(str) (reinterpret_cast<const char*>(str))
#define AS_WCHAR(str) (reinterpret_cast<wchar_t*>(str))
#define AS_CONST_WCHAR(str) (reinterpret_cast<const wchar_t*>(str))
#define CONCATENATE(e1, e2) e1 ## e2

#ifdef UNICODE
// Return L"s"
#define CONSTRUCT_SQLSTR(s) CONCATENATE(L, s)
#else
// No-op
#define CONSTRUCT_SQLSTR(s) s
#endif

#if defined(__APPLE__) || defined(__linux__)
#define strcmp_case_insensitive(str1, str2) strcasecmp(str1, str2)
#else
#define strcmp_case_insensitive(str1, str2) strcmpi(str1, str2)
#endif

#ifdef UNICODE
typedef std::wstring SQLSTR;
#else
typedef std::string SQLSTR;
#endif

class StringHelper {
public:
    static std::wstring ToWstring(const std::string& src) {
        if (src.empty()) {
            return std::wstring();
        }
        return std::wstring(src.begin(), src.end());
    }

    static std::string ToString(SQLTCHAR *src) {
        #ifdef UNICODE
        std::wstring wstr = AS_WCHAR(src);
        return std::string(wstr.begin(), wstr.end());
        #else
        return std::string(AS_CHAR(src));
        #endif
    }

    static std::string ToString(const std::wstring& src) {
        if (src.empty()) {
            return std::string();
        }
        return std::string(src.begin(), src.end());
    }

    static std::string ToString(const std::string& src) {
        return src;
    }
};

#endif  // STRING_HELPER_H_

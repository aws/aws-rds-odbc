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

#include <codecvt>
#include <locale>
#include <string.h>

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

#if defined(__APPLE__) || defined(__linux__)
typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#else
typedef std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
#endif

class StringHelper {
   public:
    static std::wstring ToWstring(const std::string& src) {
        if (src.empty()) {
            return std::wstring();
        }
        return converter{}.from_bytes(src);
    }
    static std::string ToString(const std::wstring& src) {
        if (src.empty()) {
            return std::string();
        }
        return converter{}.to_bytes(src);
    }
};

#endif  // STRING_HELPER_H_

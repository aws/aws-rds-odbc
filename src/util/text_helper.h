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

#ifndef TEXT_HELPER_H_
#define TEXT_HELPER_H_

#ifdef WIN32
    #include <tchar.h>
#else
    // On non-Windows platforms:
    #ifdef UNICODE
        #define TEXT(x) L##x
        #include <cwchar> // For wide string functions like wcslen, wcscpy
    #else
        #define TEXT(x) x
        #include <cstring> // For narrow string functions like strlen, strcpy
    #endif
#endif

#endif //TEXT_HELPER_H_

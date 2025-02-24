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

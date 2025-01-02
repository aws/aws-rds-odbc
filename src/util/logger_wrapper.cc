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


#ifdef WIN32
    #include <windows.h>
#else
    #include <iconv.h>
#endif

#include "logger_wrapper.h"

void LoggerWrapper::initialize() {
#ifndef XCODE_BUILD
    static LoggerWrapper instance;
    if (!instance.init) {
        FLAGS_stderrthreshold = 4; // Disable console output
        FLAGS_timestamp_in_logfile_name = false;
        set_log_directory(logger_config::LOG_LOCATION);
        google::InitGoogleLogging(logger_config::PROGRAM_NAME.c_str());
        instance.init = true;
    }
#endif
}

std::string LoggerWrapper::convert_wchar_to_char(const wchar_t* wstr) {
    if (wstr == nullptr) {
        return "";
    }

#ifdef WIN32
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, &result[0], size_needed, nullptr, nullptr);
    result.pop_back(); // Remove the null terminator
    return result;
#else
    iconv_t conv = iconv_open("UTF-8", "WCHAR_T");
    if (reinterpret_cast<std::uintptr_t>(conv) == static_cast<std::uintptr_t>(-1)) {
        throw std::runtime_error("iconv_open failed");
    }

    size_t in_bytes_left = wcslen(wstr) * sizeof(wchar_t);
    size_t out_bytes_left = in_bytes_left * 4; // UTF-8 may use up to 4 bytes per wchar_t
    std::vector<char> out_buf(out_bytes_left);
    char* in_buf = const_cast<char *>(reinterpret_cast<const char *>(wstr));
    char* out_ptr = out_buf.data();

    if (iconv(conv, &in_buf, &in_bytes_left, &out_ptr, &out_bytes_left) == static_cast<size_t>(-1)) {
        iconv_close(conv);
        throw std::runtime_error("iconv failed");
    }

    iconv_close(conv);
    return std::string(out_buf.data(), out_buf.size() - out_bytes_left);
#endif
}

void LoggerWrapper::set_log_directory(const std::string& directory_path) {
#ifndef XCODE_BUILD
    if (!std::filesystem::exists(directory_path)) {
        std::filesystem::create_directory(directory_path);
    }
    FLAGS_log_dir = directory_path;
#endif
}

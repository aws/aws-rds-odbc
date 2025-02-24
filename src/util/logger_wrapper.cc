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


#ifdef WIN32
    #include <windows.h>
#else
    #include <iconv.h>
#endif

#include "logger_wrapper.h"

void LoggerWrapper::initialize() { initialize(logger_config::LOG_LOCATION); }

void LoggerWrapper::initialize(std::string log_location) {
#ifndef XCODE_BUILD
    static LoggerWrapper instance;
    if (!instance.init) {
        FLAGS_stderrthreshold = 4; // Disable console output
        FLAGS_timestamp_in_logfile_name = false;
        if (log_location.empty()) {
            log_location = logger_config::LOG_LOCATION;
        }
        set_log_directory(log_location);
        google::InitGoogleLogging(logger_config::PROGRAM_NAME.c_str());
        instance.init = true;
    }
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

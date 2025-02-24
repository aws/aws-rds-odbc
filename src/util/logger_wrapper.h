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

#ifndef LOGGER_WRAPPER_H_
#define LOGGER_WRAPPER_H_

#ifdef XCODE_BUILD
// Setting this to a value greater than 3 strips out all of the
// Google logging functionality
#define GOOGLE_STRIP_LOG 4
#endif /* XCODE_BUILD */

#include <filesystem>
#include <glog/logging.h>

#ifdef WIN32
    #include <windows.h>
#endif

#include <sqltypes.h>

namespace logger_config {
    const std::string PROGRAM_NAME = "aws-rds-odbc";
    const std::string LOG_LOCATION = std::filesystem::temp_directory_path().append("aws-rds-odbc").string();
}  // namespace logger_config

// Initializes glog once
class LoggerWrapper {
public:
    static void initialize();
    static void initialize(std::string log_location);

    // Prevent copy constructors
    LoggerWrapper(const LoggerWrapper&) = delete;
    LoggerWrapper(LoggerWrapper&&) = delete;
    LoggerWrapper& operator=(const LoggerWrapper&) = delete;
    LoggerWrapper& operator=(LoggerWrapper&&) = delete;

protected:
    LoggerWrapper() = default;

private:
    static void set_log_directory(const std::string& directory_path);
    bool init = false;
};

#endif // LOGGER_WRAPPER_H_

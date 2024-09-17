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

#ifndef __LOGGERWRAPPER_H__
#define __LOGGERWRAPPER_H__

#include <log4cplus/consoleappender.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/initializer.h>

#include <format>
#include <filesystem>

namespace LOGGER_CONFIG {
    const long MAX_FILE_SIZE = 5 * 1024 * 1024;
    const int MAX_BACKUP_SIZE = 5;
    const std::string LOG_LOCATION = std::filesystem::temp_directory_path().append("aws-rds-odbc.log").string();
}  // namespace LOGGER_CONFIG

class LOGGER_WRAPPER : public log4cplus::Logger {
public:
    LOGGER_WRAPPER(std::string name, log4cplus::LogLevel level = log4cplus::INFO_LOG_LEVEL);
    ~LOGGER_WRAPPER();

    void info(std::string msg);
    void debug(std::string msg);
    void warn(std::string msg);
    void error(std::string msg);
    void fatal(std::string msg);

private:
    log4cplus::Logger _logger;

    void configure(log4cplus::LogLevel logLevel);
};

#endif /* __LOGGERWRAPPER_H__ */

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

#include "logger_wrapper.h"

LOGGER_WRAPPER::LOGGER_WRAPPER(std::string name, log4cplus::LogLevel logLevel) {
    log4cplus::initialize();
    _logger = log4cplus::Logger::getInstance(LOG4CPLUS_C_STR_TO_TSTRING(name.c_str()));
    configure(logLevel);
}

LOGGER_WRAPPER::~LOGGER_WRAPPER() {
    log4cplus::Logger::shutdown();
}

void LOGGER_WRAPPER::configure(log4cplus::LogLevel logLevel) {
    log4cplus::Logger rootLogger = log4cplus::Logger::getRoot();
    rootLogger.setLogLevel(logLevel);

    log4cplus::SharedFileAppenderPtr sharedFileAppender(
        new log4cplus::RollingFileAppender(
            LOG4CPLUS_C_STR_TO_TSTRING(LOGGER_CONFIG::LOG_LOCATION), 
            LOGGER_CONFIG::MAX_FILE_SIZE, LOGGER_CONFIG::MAX_BACKUP_SIZE,
            false, true
    ));

    log4cplus::tstring pattern = LOG4CPLUS_TEXT("%D{%m/%d/%y %H:%M:%S,%Q} [%t] %-5p %c{2} - %m [%l]%n");
    sharedFileAppender->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(pattern)));

    rootLogger.addAppender(log4cplus::SharedAppenderPtr(sharedFileAppender.get()));
}

void LOGGER_WRAPPER::info(std::string msg) {
    LOG4CPLUS_INFO(_logger, LOG4CPLUS_C_STR_TO_TSTRING(msg.c_str()));
}

void LOGGER_WRAPPER::debug(std::string msg) {
    LOG4CPLUS_DEBUG(_logger, LOG4CPLUS_C_STR_TO_TSTRING(msg.c_str()));
}

void LOGGER_WRAPPER::warn(std::string msg) {
    LOG4CPLUS_WARN(_logger, LOG4CPLUS_C_STR_TO_TSTRING(msg.c_str()));
}

void LOGGER_WRAPPER::error(std::string msg) {
    LOG4CPLUS_ERROR(_logger, LOG4CPLUS_C_STR_TO_TSTRING(msg.c_str()));
}

void LOGGER_WRAPPER::fatal(std::string msg) {
    LOG4CPLUS_FATAL(_logger, LOG4CPLUS_C_STR_TO_TSTRING(msg.c_str()));
}

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

#ifdef WIN32
    #include <windows.h>
#else
    #include <iconv.h>
#endif

#include "logger_wrapper.h"

void LoggerWrapper::initialize() {
    initialize(logger_config::LOG_LOCATION, 4);
}

void LoggerWrapper::initialize(std::string log_location, int threshold) {
    threshold = threshold >= 0 ? threshold : 4; // Set to 4 to disable console output.
    static LoggerWrapper instance;
    if (!instance.init) {
        FLAGS_stderrthreshold = threshold;
        FLAGS_timestamp_in_logfile_name = false;
        if (log_location.empty()) {
            log_location = logger_config::LOG_LOCATION;
        }
        set_log_directory(log_location);
        google::InitGoogleLogging(logger_config::PROGRAM_NAME.c_str());
        instance.init = true;
    }
}

void LoggerWrapper::set_log_directory(const std::string& directory_path) {
    if (!std::filesystem::exists(directory_path)) {
        std::filesystem::create_directory(directory_path);
    }
    FLAGS_log_dir = directory_path;
}

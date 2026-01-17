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

#include <glog/logging.h>

#include "logger_wrapper.h"

static LoggerWrapper instance;

void LoggerWrapper::Initialize() {
    Initialize(logger_config::LOG_LOCATION, 4);
}

void LoggerWrapper::Initialize(std::string log_location, int threshold) {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    threshold = threshold >= 0 ? threshold : 4; // Set to 4 to disable console output.
    
    if (0 == logger_init_count++) {
        if (glog_initialized.load()) {
            // Shutdown and reinitialize with new settings
            google::ShutdownGoogleLogging();
        }

        FLAGS_logtostderr = false; // Re-enable file logging if it was previously disabled
        FLAGS_stderrthreshold = threshold;
        FLAGS_timestamp_in_logfile_name = false;
        FLAGS_minloglevel = 0;
        if (log_location.empty()) {
            log_location = logger_config::LOG_LOCATION;
        }
        set_log_directory(log_location);
        google::InitGoogleLogging(logger_config::PROGRAM_NAME.c_str());
        glog_initialized.store(true);
    }
}

void LoggerWrapper::InitializeMinimal() {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    if (!glog_initialized.load()) {
        // Initialize glog with minimal settings to suppress all output
        FLAGS_minloglevel = 4;
        FLAGS_stderrthreshold = 4; // Suppress stderr
        FLAGS_logtostderr = true; // Suppress file logging
        google::InitGoogleLogging(logger_config::PROGRAM_NAME.c_str());
        glog_initialized.store(true);
    }
}

bool LoggerWrapper::IsInitialized() {
    return glog_initialized.load();
}

void LoggerWrapper::Shutdown() {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    if (logger_init_count > 0 && --logger_init_count == 0) {
        google::ShutdownGoogleLogging();
        glog_initialized.store(false, std::memory_order_release);
        // Reset to suppress stderr output when no logging is active
        FLAGS_stderrthreshold = 4;
        FLAGS_minloglevel = 4;
    }
}

void LoggerWrapper::set_log_directory(const std::string& directory_path) {
    if (!std::filesystem::exists(directory_path)) {
        std::filesystem::create_directory(directory_path);
    }
    FLAGS_log_dir = directory_path;
}

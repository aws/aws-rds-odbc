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

#ifndef LOGGER_WRAPPER_H_
#define LOGGER_WRAPPER_H_

#ifdef XCODE_BUILD
// Setting this to a value greater than 3 strips out all of the
// Google logging functionality
#define GOOGLE_STRIP_LOG 4
#endif /* XCODE_BUILD */

#include <atomic>
#include <filesystem>
#include <mutex>

namespace logger_config {
    const std::string PROGRAM_NAME = "aws-rds-odbc";
    const std::string LOG_LOCATION = std::filesystem::temp_directory_path().append(PROGRAM_NAME).string();
}  // namespace logger_config

// Initializes glog once
class LoggerWrapper {
public:
    LoggerWrapper() = default;

    static void Initialize();
    static void Initialize(std::string log_location, int threshold);
    
    // Initialize glog with minimal settings (suppresses all output)
    static void InitializeMinimal();
    
    // Check if glog has been initialized
    static bool IsInitialized();

    static void Shutdown();

    // Prevent copy constructors
    LoggerWrapper(const LoggerWrapper&) = delete;
    LoggerWrapper(LoggerWrapper&&) = delete;
    LoggerWrapper& operator=(const LoggerWrapper&) = delete;
    LoggerWrapper& operator=(LoggerWrapper&&) = delete;

private:
    static void set_log_directory(const std::string& directory_path);
    
    static inline int logger_init_count = 0;
    static inline std::atomic<bool> glog_initialized = false;
    static inline std::mutex logger_mutex;
};

#endif // LOGGER_WRAPPER_H_

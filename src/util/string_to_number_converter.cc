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

#include "string_to_number_converter.h"

#include <cerrno>
#include <cstdlib>
#include <glog/logging.h>

int StringToNumberConverter::Atoi(const char* str) {
    char* endptr;
    errno = 0;

    const int64_t val = std::strtol(str, &endptr, StringToNumberConverter::DECIMAL_BASE);
    if (errno != 0) {
        LOG(ERROR) << "Got the following error number from strtol: " << errno;
    }

    if (*endptr != '\0') {
        LOG(ERROR) << "There is a non-alphanumerical error passed in to strtol";
    }
    return static_cast<int>(val);
}

double StringToNumberConverter::Atof(const char* str) {
    char* endptr;
    errno = 0;

    const double val = std::strtof(str, &endptr);
    if (errno != 0) {
        LOG(ERROR) << "Got the following error number from strtol: " << errno;
    }

    if (*endptr != '\0') {
        LOG(ERROR) << "There is a non-alphanumerical error passed in to strtol";
    }
    return val;
}

int64_t StringToNumberConverter::Atol(const char* str) {
    char* endptr;
    errno = 0;

    const int64_t val = std::strtol(str, &endptr, StringToNumberConverter::DECIMAL_BASE);
    if (errno != 0) {
        LOG(ERROR) << "Got the following error number from strtol: " << errno;
    }

    if (*endptr != '\0') {
        LOG(ERROR) << "There is a non-alphanumerical error passed in to strtol";
    }
    return val;
}

int64_t StringToNumberConverter::Atoll(const char* str) {
    char* endptr;
    errno = 0;

    const int64_t val = std::strtoll(str, &endptr, StringToNumberConverter::DECIMAL_BASE);
    if (errno != 0) {
        LOG(ERROR) << "Got the following error number from strtoll: " << errno;
    }

    if (*endptr != '\0') {
        LOG(ERROR) << "There is a non-alphanumerical error passed in to strtoll";
    }
    return val;
}

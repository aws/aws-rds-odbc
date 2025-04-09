//
// Created by Favian Samatha on 2025-04-09.
//

#include "string_to_number_converter.h"

#include <glog/logging.h>
#include <cerrno>
#include <cstdlib>

int StringToNumberConverter::Atoi(const char* str) {
    char* endptr;
    errno = 0;

    const long val = std::strtol(str, &endptr, StringToNumberConverter::DECIMAL_BASE);
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

long int StringToNumberConverter::Atol(const char* str) {
    char* endptr;
    errno = 0;

    const long val = std::strtol(str, &endptr, StringToNumberConverter::DECIMAL_BASE);
    if (errno != 0) {
        LOG(ERROR) << "Got the following error number from strtol: " << errno;
    }

    if (*endptr != '\0') {
        LOG(ERROR) << "There is a non-alphanumerical error passed in to strtol";
    }
    return val;
}

long long int StringToNumberConverter::Atoll(const char* str) {
    char* endptr;
    errno = 0;

    long long val = std::strtol(str, &endptr, StringToNumberConverter::DECIMAL_BASE);
    if (errno != 0) {
        LOG(ERROR) << "Got the following error number from strtoll: " << errno;
    }

    if (*endptr != '\0') {
        LOG(ERROR) << "There is a non-alphanumerical error passed in to strtoll";
    }
    return val;
}

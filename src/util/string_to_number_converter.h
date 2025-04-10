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

#ifndef STRING_TO_NUMBER_CONVERTER_H
#define STRING_TO_NUMBER_CONVERTER_H

#include <stdint.h>

class StringToNumberConverter {
public:
    static const int DECIMAL_BASE = 10;

    static int Atoi(const char* str);
    static double Atof(const char* str);
    static int64_t Atol(const char* str);
    static int64_t Atoll(const char* str);
};

#endif //STRING_TO_NUMBER_CONVERTER_H

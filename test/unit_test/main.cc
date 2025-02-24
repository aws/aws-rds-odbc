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

#include "gtest/gtest.h"
#include <logger_wrapper.h>

int main(int argc, char** argv) {
  LoggerWrapper::initialize();

#ifdef WIN32
#ifdef _DEBUG
  // Enable CRT for detecting memory leaks
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#endif
#ifdef __APPLE__
  // Enable malloc logging for detecting memory leaks.
  system("export MallocStackLogging=1");
#endif
  testing::internal::CaptureStdout();
  ::testing::InitGoogleTest(&argc, argv);
  int failures = RUN_ALL_TESTS();
  std::string output = testing::internal::GetCapturedStdout();
  std::cout << output << std::endl;
  std::cout << (failures ? "Not all tests passed." : "All tests passed")
            << std::endl;
#ifdef __APPLE__
  // Disable malloc logging
  system("unset MallocStackLogging");
#endif
  return failures;
}

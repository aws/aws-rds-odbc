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

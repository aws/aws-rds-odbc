# Build Library and Run Unit Tests

## Table of Contents
- [Windows](#windows)
    - [Prerequisites](#prerequisites)
    - [Build the library and run the tests](#build-the-library-and-run-the-tests)
    - [Debugging with Visual Studio](#debugging-with-visual-studio)
- [macOS](#macos)
    - [Prerequisites](#prerequisites-1)
    - [Build the library and run tests in a terminal](#build-the-library-and-run-tests-in-a-terminal)
    - [Debugging with Xcode](#debugging-with-xcode)

## Windows

### Prerequisites
1. Download the CMake Windows x64 Installer from https://cmake.org/download/ and install CMake using the installer. When going through the install, ensure that adding CMake to the PATH is selected.
1. Refer to [Install Microsoft Visual Studio](./InstallMicrosoftVisualStudio.md) to install Microsoft Visual Studio.

### Build the library and run the tests
1. Open a PowerShell terminal and change to the root repository folder.
1. If the AWS SDK has not been built yet, run the following to build the AWS SDK,
   replacing `<configuration>` below with either `Debug` or `Release`.
   ```PowerShell
   .\scripts\build_aws_sdk_win.ps1 x64 <configuration> OFF "Visual Studio 17 2022"
   ```
1. Run the following to build the ANSI and Unicode versions of the library and the unit tests, replacing `<configuration>`
   below with either `Debug` or `Release`. This must match the configuration used
   to build the AWS SDK above.

   **ANSI**
   ```PowerShell
   cmake -S . -B build_ansi -DENABLE_UNIT_TESTS=TRUE
   cmake --build build_ansi
   ```

   **Unicode**
   ```PowerShell
   cmake -S . -B build_unicode -DUNICODE_BUILD=ON -DENABLE_UNIT_TESTS=TRUE
   cmake --build build_unicode
   ```

1. Run the following from the repository root folder.

   **ANSI**
   ```PowerShell
   build_ansi\test\unit_test\bin\Release\unit_test.exe
   ```

   **Unicode**
   ```PowerShell
   build_unicode\test\unit_test\bin\Release\unit_test.exe
   ```

### Debugging with Visual Studio
1. Follow the steps under [Build the library and run the tests](#build-library-and-run-unit-tests) above,
   adding `-DCMAKE_BUILD_TYPE=Debug` to the first `cmake` command under ANSI and Unicode.
1. Start Visual Studio 2022.
1. Open one of these solutions
   - **ANSI**: `<root repository folder>\build_ansi\test\unit_test\unit_test.sln`.
   - **Unicode** `<root repository folder>\build_unicode\test\unit_test\unit_test.sln`.
1. Set breakpoints in any of the source files listed in the solution.
1. Inside the solution tree, right-click on `unit_test` and select Debug -> Start New Instance.<br>If prompted to build other projects, select Yes.
1. After the build completes, the debugger will start.

## macOS

### Prerequisites
1. Install [Homebrew](https://brew.sh/). If Homebrew is already installed, run the following command to install the latest updates.
   ```bash
   brew update && brew upgrade && brew cleanup
   ```
1. Run the following command to install the build dependencies.
   ```bash
   brew install cmake googletest unixodbc
   ```
1. Install the Xcode Command Line tools using one of the following methods.
   1. Install Xcode from the App Store
   1. Run the following command
   ```bash
   xcode-select --install
   ```

### Build the library and run tests in a terminal
1. Open a terminal and change to the root repository folder.
1. Run the following to build the AWS SDK.
   ```bash
   scripts/build_aws_sdk_macos Release
   ```
1. Run the following to build the ANSI and Unicode versions of the library and the unit tests

   **ANSI**
   ```bash
   cmake -S . -B build_ansi -DENABLE_UNIT_TESTS=TRUE
   cmake --build build_ansi
   ```

   **Unicode**
   ```bash
   cmake -S . -B build_unicode -DUNICODE_BUILD=ON -DENABLE_UNIT_TESTS=TRUE
   cmake --build build_unicode
   ```

1. Run the following to run the unit tests.

   **ANSI**
   ```bash
   build_ansi/test/unit_test/bin/unit_test
   ```

   **Unicode**
   ```bash
   build_unicode/test/unit_test/bin/unit_test
   ```

### Debugging with Xcode
1. Start Xcode.
1. Open the project, `<root repository folder>/test/unit_test/aws-rds-odbc-unit-tests.xcodeproj`.
1. Set breakpoints in any of the source files listed in the project tree.
1. Press ⌘+R or select Run from the Product menu. This will build the library, the tests and start the debug execution of the tests.
# Build and Run Tests

## Windows

### Build and run tests
1. Open a PowerShell terminal and change to the root repository folder.
1. If the AWS SDK has not been built yet, run the following to build the AWS SDK,
   replacing `<configuration>` below with either `Debug` or `Release`.
   ```PowerShell
   .\scripts\build_aws_sdk_win.ps1 x64 <configuration> OFF "Visual Studio 17 2022"
   ```
1. Run the following to build the library and the unit tests, replacing `<configuration>`
   below with either `Debug` or `Release`. This must match the configuration used
   to build the AWS SDK above.
   ```PowerShell
   cmake -S . -B build -DENABLE_UNIT_TESTS=TRUE
   cmake --build build --config <configuration>
   ```
1. Run the following from the repository root folder.
   ```PowerShell
   build\test\unit_test\bin\Release\unit_test.exe
   ```

### Debugging with Visual Studio
1. Follow the steps under "Build and run tests" above, replacing `<configuration>` with `Debug`.
1. Start Visual Studio 2022.
1. Open the solution, `<root repository folder>\build\test\unit_test\unit_test.sln`.
1. Set breakpoints in any of the projects listed in the solution.
1. Inside the solution tree, right-click on `unit_test` and select Debug -> Start New Instance.<br>If prompted to build other projects, select Yes.
1. After the build completes, the debugger will start.

## macOS

### Prerequisites
Open a terminal and run the following.
1. `brew install googletest unixodbc`

### Build and run tests
1. Open a terminal and change to the root repository folder.
1. If the AWS SDK has not been built yet, run the following to build the AWS SDK,
   replacing `<configuration>` below either `Debug` or `Release`.
   ```bash
   scripts/build_aws_sdk_macos <configuration>
   ```
1. Run the following to build the library and the unit tests, replacing `<configuration>`
   below with either `Debug` or `Release`. This must match the configuration used
   to build the AWS SDK above.
   ```bash
   cmake -S . -B build -DENABLE_UNIT_TESTS=TRUE
   cmake --build build --config <configuration>
   ```
1. Run the following to run the unit tests.
   ```bash
   build/test/unit_test/bin/unit_test
   ```

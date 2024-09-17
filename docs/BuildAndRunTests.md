# Build and Run Tests

## Unit Tests

### Build and Run
1. Open a PowerShell terminal.
2. Change to the root repository folder.
3. If the AWS SDK has not been built yet, run the following to build the AWS SDK.
   ```PowerShell
   .\scripts\build_aws_sdk_win.ps1 x64 Release OFF "Visual Studio 17 2022"
   ```
4. Run the following to build the library and the unit tests.
   ```PowerShell
   cmake -S . -B build -DENABLE_UNIT_TESTS=TRUE
   cmake --build build --config Release
   ```
5. Run the following from the repository root folder.
   ```PowerShell
   cd build\test\unit_test\bin\Release
   .\unit_test.exe
   ```

### Debugging with Visual Studio
1. Follow the steps under [Build and Run](#build-and-run), replacing `Release` with `Debug`.
2. Start Visual Studio 2022.
3. Open the solution, `<root repository folder>\build\test\unit_test\unit_test.sln`.
4. Set breakpoints in any of the projects listed in the solution.
5. Inside the solution tree, right-click on `unit_test` and select Debug -> Start New Instance.<br>If prompted to build other projects, select Yes.
6. After the build completes, the debugger will start.

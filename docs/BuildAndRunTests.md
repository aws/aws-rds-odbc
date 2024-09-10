# Build and Run Tests

## Unit Tests
1. In repository root folder, build cmake with the following flag `-DENABLE_UNIT_TESTS=TRUE`
e.g.
```cmake
cmake -S . -B build -DENABLE_UNIT_TESTS=TRUE
cmake --build build
```

2. Then navigate to the built binary and run executable
e.g.
```
cd build\test\unit_test\bin\Debug
.\unit_test.exe
```

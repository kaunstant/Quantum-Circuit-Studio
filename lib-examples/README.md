# QCS library examples (v0.3.5)

These are standalone consumers of the installed QCS library. Each `.cpp` file is an executable and includes the public umbrella header:

```cpp
#include <QCS>
```

## Build on Windows

Run these commands from the repository root in PowerShell:

```powershell
cmake -S engine -B engine/build/release -DCMAKE_INSTALL_PREFIX="$PWD/engine/build/qcs-install/release"
cmake --build engine/build/release --config Release --target quantum_engine
cmake --install engine/build/release --config Release
cmake -S lib-examples -B engine/build/lib-examples -DCMAKE_PREFIX_PATH="$PWD/engine/build/qcs-install/release"
cmake --build engine/build/lib-examples --config Release
```

The installed package contains `include/QCS`, the public headers, `quantum_engine.lib`, and the CMake package files. A separate project can use:

```cmake
find_package(QCS CONFIG REQUIRED)
add_executable(my_program main.cpp)
target_link_libraries(my_program PRIVATE QCS::QCS)
```

Set `CMAKE_PREFIX_PATH` to the selected install directory, or pass the package directory directly with `-DQCS_DIR=<install-prefix>/lib/cmake/QCS`. The engine launcher creates separate `debug` and `release` install prefixes so consumers never mix configurations.
@echo off

REM Create build directory
mkdir build
cd build

REM Get the current git tag
for /f "tokens=*" %%i in ('git describe --tags') do set GIT_TAG=%%i

REM Run CMake to configure the project with MSVC in release mode
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=" -DGIT_TAG=\\\"%GIT_TAG%\\\"" ..

REM Build the project
cmake --build . --config Release

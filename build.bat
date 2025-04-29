@echo off
setlocal

:: Create build directory
if not exist build mkdir build

:: Configure and build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DX86EMU_USE_86BOX=ON -DX86EMU_USE_MAME=ON ..
cmake --build . --config Release --parallel

echo Build completed. Binaries are in build\bin\Release
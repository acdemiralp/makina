

if not exist "build" mkdir build
cd build
if not exist "vcpkg" git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
if not exist "vcpkg.exe" call bootstrap-vcpkg.bat

set VCPKG_DEFAULT_TRIPLET=x64-windows
vcpkg install --recurse --overlay-ports=../../vcpkg/overlay_ports cxxopts doctest eigen3 nlohmann-json openxr-loader sdl2 spdlog stb taskflow thrust
cd ..

cmake -Ax64 -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --clean-first --target ALL_BUILD --config Release --parallel 8
cd ..
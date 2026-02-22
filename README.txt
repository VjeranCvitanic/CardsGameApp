CardsGame engine + gRPC server

Build instructions (macOS + Windows)

Requirements
- CMake >= 3.16
- C++ compiler with C++20 support for engine and C++17 for gRPC app
- Protobuf + gRPC C++ libraries

macOS (Homebrew)
1) Install dependencies:
   brew install cmake protobuf grpc
2) Configure:
   cmake -S . -B build
3) Build:
   cmake --build build

Windows (vcpkg + Visual Studio 2022)
1) Install dependencies via vcpkg (x64 Native Tools prompt):
   vcpkg install grpc protobuf --triplet x64-windows
2) Configure:
   cmake -S . -B build -G "Visual Studio 17 2022" \
     -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake \
     -DVCPKG_TARGET_TRIPLET=x64-windows
3) Build Release:
   cmake --build build --config Release

Run (from repo root)
- Server:
  - macOS: ./build/gRPC/server
  - Windows: .\build\gRPC\Release\server.exe
- Client:
  - macOS: ./build/gRPC/client <name> <tressette|briscola> <2|4> <ai|human>
  - Windows: .\build\gRPC\Release\client.exe <name> <tressette|briscola> <2|4> <ai|human>

Legacy notes: see gRPC/doc/doc.txt
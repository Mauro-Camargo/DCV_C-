cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)

project(dcv-ldi-extension-cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  # Static CRT, matching the x64-windows-static-release vcpkg triplet (see
  # triplets/), so the resulting exe carries no external runtime DLL
  # dependency - the C++ equivalent of the .NET project's self-contained
  # single-file publish.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(protobuf CONFIG REQUIRED)

add_executable(dcv-ldi-extension-cpp
  src/main.cpp
  src/LdiPayload.cpp
  src/ClientRole.cpp
  src/ServerRole.cpp
  src/DcvSdk/Reader.cpp
  src/DcvSdk/Writer.cpp
  src/DcvSdk/Processor.cpp
  src/DcvSdk/VirtualChannel.cpp
)

target_compile_definitions(dcv-ldi-extension-cpp PRIVATE
  _CRT_SECURE_NO_WARNINGS
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

target_include_directories(dcv-ldi-extension-cpp PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

protobuf_generate(
  TARGET dcv-ldi-extension-cpp
  PROTOS ${CMAKE_SOURCE_DIR}/proto/extensions.proto
  IMPORT_DIRS ${CMAKE_SOURCE_DIR}/proto
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(dcv-ldi-extension-cpp PRIVATE
  protobuf::libprotobuf
  ws2_32
  iphlpapi
  rpcrt4
  shell32
  ole32
)

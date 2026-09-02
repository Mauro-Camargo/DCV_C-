set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# We only ever build the Release configuration (see build-and-sign.yml), so
# skip building/compiling protobuf's Debug variant too - vcpkg builds both
# by default, which roughly doubles CI time for no benefit here.
set(VCPKG_BUILD_TYPE release)

set(VCPKG_CMAKE_SYSTEM_NAME Windows)

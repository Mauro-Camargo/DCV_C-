set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# We only ever build the Release configuration (see build-and-sign.yml), so
# skip building/compiling protobuf's Debug variant too - vcpkg builds both
# by default, which roughly doubles CI time for no benefit here.
set(VCPKG_BUILD_TYPE release)

# Deliberately NOT setting VCPKG_CMAKE_SYSTEM_NAME: leaving it unset is what
# tells vcpkg this is a plain native desktop build. Setting it to "Windows"
# (even though that's technically correct) flips vcpkg's internal
# cross-compiling detection path, which then fails to detect the active
# compiler ("vcpkg was unable to detect the active compiler's information").

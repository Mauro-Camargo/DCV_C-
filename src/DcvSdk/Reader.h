#pragma once

#include <windows.h>

#include <cstdint>

#include "extensions.pb.h"

namespace DcvSdk {

// Reads length-prefixed DcvMessage frames from DCV's stdout-to-extension pipe
// (4-byte little-endian size header + protobuf body), mirroring
// DcvLdiExtension.DcvSdk.Reader from the C# implementation.
class Reader {
public:
    explicit Reader(HANDLE input);

    // Blocks until a full message is available. On EOF (DCV closed the
    // pipe), terminates the process immediately via ExitProcess(0) - this
    // matches the C# reader, which relies on the same shutdown signal.
    dcv::extensions::DcvMessage ReceiveMessage();

private:
    HANDLE input_;

    bool ReadExact(uint8_t* buffer, DWORD size);
};

}  // namespace DcvSdk

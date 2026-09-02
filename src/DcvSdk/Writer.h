#pragma once

#include <windows.h>

#include <cstdint>
#include <mutex>

#include "extensions.pb.h"

namespace DcvSdk {

// Writes length-prefixed ExtensionMessage frames to DCV's stdin-from-extension
// pipe, mirroring DcvLdiExtension.DcvSdk.Writer from the C# implementation.
class Writer {
public:
    explicit Writer(HANDLE output);

    void SendRequest(const dcv::extensions::Request& request);

private:
    HANDLE output_;
    std::mutex mutex_;

    bool WriteExact(const uint8_t* buffer, DWORD size);
};

}  // namespace DcvSdk

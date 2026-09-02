#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace DcvSdk {

// Client side of a DCV virtual channel relay (a named pipe), mirroring
// DcvLdiExtension.DcvSdk.VirtualChannel from the C# implementation.
class VirtualChannel {
public:
    // Connects to `relayPath` (the full named-pipe path returned by DCV in
    // SetupVirtualChannelResponse.relay_path) and immediately writes
    // `authToken` as the first bytes on the pipe, as DCV requires.
    static VirtualChannel Connect(const std::string& relayPath, const std::string& authToken);

    VirtualChannel(VirtualChannel&& other) noexcept;
    VirtualChannel& operator=(VirtualChannel&& other) noexcept;
    VirtualChannel(const VirtualChannel&) = delete;
    VirtualChannel& operator=(const VirtualChannel&) = delete;
    ~VirtualChannel();

    // Single read of whatever is currently available (up to 4 KiB), matching
    // the C# version's single ReadAsync call. Returns an empty vector on
    // EOF/error.
    std::vector<uint8_t> Read();

    void Write(const std::vector<uint8_t>& data);
    void Write(const std::string& data);

private:
    explicit VirtualChannel(HANDLE pipe);

    HANDLE pipe_ = INVALID_HANDLE_VALUE;
};

}  // namespace DcvSdk

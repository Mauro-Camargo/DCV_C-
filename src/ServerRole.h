#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "DcvSdk/VirtualChannel.h"

namespace DcvLdiExtension {

// Mirrors DcvLdiExtension.ServerRole from the C# implementation.
class ServerRole {
public:
    static void Run(DcvSdk::VirtualChannel& channel, bool channelReady, std::atomic<bool>& cancelled);

private:
    static void ServeLdiPipe(std::atomic<bool>& cancelled);

    static std::mutex payloadMutex_;
    static std::vector<uint8_t> currentPayload_;
};

}  // namespace DcvLdiExtension

#pragma once

#include <atomic>
#include <string>

#include "DcvSdk/VirtualChannel.h"
#include "LdiPayload.h"

namespace DcvLdiExtension {

// Mirrors DcvLdiExtension.ClientRole from the C# implementation.
class ClientRole {
public:
    // Sends an LDI payload over the channel every 30s until `cancelled`.
    static void Run(DcvSdk::VirtualChannel& channel, std::atomic<bool>& cancelled);

    // Standalone mode: no DCV IPC available (registry-launched on the
    // client side without stdin/stdout). Keeps the process alive so the
    // WorkSpaces client doesn't report it as crashed.
    static void RunStandalone(std::atomic<bool>& cancelled);

private:
    static LdiPayload CollectLdi();
    static void GetPrimaryInterface(std::string& ipv4, std::string& adapterType);
};

}  // namespace DcvLdiExtension

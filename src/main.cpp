// Entry point mirroring Program.cs from the C# dcv-ldi-extension: detects
// whether this process is running as the DCV server-side or client-side
// extension, sets up a virtual channel, and hands off to ClientRole or
// ServerRole.
//
// Unlike the C# version (async/await), this uses blocking calls plus a
// couple of dedicated threads - the same "simple approach using synchronous
// IO" the official AWS C++ SDK sample itself documents.

#include <windows.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "ClientRole.h"
#include "DcvSdk/Processor.h"
#include "DcvSdk/VirtualChannel.h"
#include "ServerRole.h"
#include "extensions.pb.h"

using namespace dcv::extensions;
using namespace DcvSdk;
using namespace DcvLdiExtension;

namespace {

const std::string kChannelName = "com.amazon.dcv-ldi-extension";
std::atomic<bool> g_cancelled{false};

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            g_cancelled = true;
            return TRUE;
        default:
            return FALSE;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-V")) {
        std::cout << "dcv-ldi-extension-cpp 1.0.0" << std::endl;
        return 0;
    }

    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    Processor processor(stdinHandle, stdoutHandle);

    // Determine role - if DCV doesn't respond within 5s, assume client
    // launched without IPC (registry-launched standalone collector).
    auto dcvInfo = processor.GetDcvInfo(std::chrono::seconds(5));
    if (!dcvInfo.has_value()) {
        ClientRole::RunStandalone(g_cancelled);
        return 0;
    }

    bool isServer = dcvInfo->dcv_role() == GetDcvInfoResponse::Server;

    SetupVirtualChannelResponse svcResp;
    try {
        svcResp = processor.SetupVirtualChannel(kChannelName);
    } catch (const std::exception&) {
        return 1;
    }

    std::optional<VirtualChannel> channelOpt;
    try {
        channelOpt.emplace(VirtualChannel::Connect(svcResp.relay_path(), svcResp.virtual_channel_auth_token()));
    } catch (const std::exception&) {
        return 1;
    }
    VirtualChannel& channel = *channelOpt;

    // Ready with timeout (30s) - the other side may never connect.
    bool ready = processor.WaitForReadyEvent(std::chrono::seconds(30));

    // Stop the active role as soon as DCV reports the channel closed.
    std::thread closedWatcher([&processor] {
        processor.WaitForClosedEvent();
        g_cancelled = true;
    });

    if (isServer) {
        ServerRole::Run(channel, ready, g_cancelled);
    } else {
        ClientRole::Run(channel, g_cancelled);
    }

    closedWatcher.detach();

    try {
        processor.CloseVirtualChannel(kChannelName);
    } catch (...) {
    }

    return 0;
}

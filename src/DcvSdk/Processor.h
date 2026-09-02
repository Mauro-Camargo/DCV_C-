#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "Reader.h"
#include "Writer.h"
#include "extensions.pb.h"

namespace DcvSdk {

// Request/response and event dispatch over the DCV extension stdin/stdout
// protocol, mirroring DcvLdiExtension.DcvSdk.Processor from the C#
// implementation. The C# version uses async/await with TaskCompletionSource;
// this port uses a background thread plus std::promise/std::future, which is
// the synchronous-IO style the official AWS C++ sample itself uses.
class Processor {
public:
    Processor(HANDLE input, HANDLE output);
    ~Processor();

    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    // Returns std::nullopt if no response arrives within `timeout`.
    std::optional<dcv::extensions::GetDcvInfoResponse> GetDcvInfo(std::chrono::milliseconds timeout);

    // Blocks until a response arrives. Throws std::runtime_error on failure.
    dcv::extensions::SetupVirtualChannelResponse SetupVirtualChannel(const std::string& channelName);

    // Best-effort; gives up silently after a short timeout.
    void CloseVirtualChannel(const std::string& channelName);

    // Returns true if the ready event arrived within `timeout`.
    bool WaitForReadyEvent(std::chrono::milliseconds timeout);

    // Blocks until the closed event arrives. Intended to run on a dedicated
    // watcher thread.
    void WaitForClosedEvent();

private:
    Reader reader_;
    Writer writer_;
    std::thread readThread_;
    std::atomic<bool> stop_{false};
    std::atomic<long long> nextRequestId_{1};

    std::mutex pendingMutex_;
    std::map<std::string, std::promise<dcv::extensions::Response>> pending_;

    std::promise<void> readyPromise_;
    std::future<void> readyFuture_;
    std::promise<void> closedPromise_;
    std::future<void> closedFuture_;

    std::string NextRequestId();
    dcv::extensions::Response SendAndWait(const dcv::extensions::Request& request);
    std::optional<dcv::extensions::Response> SendAndWait(const dcv::extensions::Request& request,
                                                          std::chrono::milliseconds timeout);

    void ReadLoop();
    void HandleResponse(const dcv::extensions::Response& response);
    void HandleEvent(const dcv::extensions::Event& event);
};

}  // namespace DcvSdk

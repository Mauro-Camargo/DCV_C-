#include "Processor.h"

#include <stdexcept>

using dcv::extensions::DcvMessage;
using dcv::extensions::Event;
using dcv::extensions::GetDcvInfoResponse;
using dcv::extensions::Request;
using dcv::extensions::Response;
using dcv::extensions::SetupVirtualChannelResponse;

namespace DcvSdk {

Processor::Processor(HANDLE input, HANDLE output)
    : reader_(input), writer_(output), readyFuture_(readyPromise_.get_future()), closedFuture_(closedPromise_.get_future()) {
    readThread_ = std::thread(&Processor::ReadLoop, this);
}

Processor::~Processor() {
    stop_ = true;
    // The read thread is blocked in a synchronous ReadFile call on stdin;
    // there is no portable way to interrupt that short of closing the
    // handle. In practice the process is being torn down here anyway (DCV
    // owns our lifetime), so we detach instead of risking a hang in the
    // destructor.
    if (readThread_.joinable()) {
        readThread_.detach();
    }
}

std::string Processor::NextRequestId() {
    return std::to_string(nextRequestId_.fetch_add(1));
}

Response Processor::SendAndWait(const Request& request) {
    std::promise<Response> promise;
    auto future = promise.get_future();
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.emplace(request.request_id(), std::move(promise));
    }
    writer_.SendRequest(request);
    return future.get();
}

std::optional<Response> Processor::SendAndWait(const Request& request, std::chrono::milliseconds timeout) {
    std::promise<Response> promise;
    auto future = promise.get_future();
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.emplace(request.request_id(), std::move(promise));
    }
    writer_.SendRequest(request);

    if (future.wait_for(timeout) != std::future_status::ready) {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.erase(request.request_id());
        return std::nullopt;
    }
    return future.get();
}

std::optional<GetDcvInfoResponse> Processor::GetDcvInfo(std::chrono::milliseconds timeout) {
    Request request;
    request.set_request_id(NextRequestId());
    request.mutable_get_dcv_info_request();

    auto response = SendAndWait(request, timeout);
    if (!response.has_value()) {
        return std::nullopt;
    }
    return response->get_dcv_info_response();
}

SetupVirtualChannelResponse Processor::SetupVirtualChannel(const std::string& channelName) {
    Request request;
    request.set_request_id(NextRequestId());
    auto* setup = request.mutable_setup_virtual_channel_request();
    setup->set_virtual_channel_name(channelName);
    setup->set_relay_client_process_id(static_cast<int64_t>(GetCurrentProcessId()));

    Response response = SendAndWait(request);
    if (response.status() != Response::SUCCESS) {
        throw std::runtime_error("SetupVirtualChannel failed with status " + std::to_string(response.status()));
    }
    return response.setup_virtual_channel_response();
}

void Processor::CloseVirtualChannel(const std::string& channelName) {
    Request request;
    request.set_request_id(NextRequestId());
    auto* close = request.mutable_close_virtual_channel_request();
    close->set_virtual_channel_name(channelName);

    // Best-effort: DCV may already be gone, so don't block indefinitely.
    SendAndWait(request, std::chrono::seconds(3));
}

bool Processor::WaitForReadyEvent(std::chrono::milliseconds timeout) {
    return readyFuture_.wait_for(timeout) == std::future_status::ready;
}

void Processor::WaitForClosedEvent() {
    closedFuture_.wait();
}

void Processor::ReadLoop() {
    try {
        while (!stop_) {
            DcvMessage msg = reader_.ReceiveMessage();
            switch (msg.msg_case()) {
                case DcvMessage::kResponse:
                    HandleResponse(msg.response());
                    break;
                case DcvMessage::kEvent:
                    HandleEvent(msg.event());
                    break;
                default:
                    break;
            }
        }
    } catch (...) {
        // Mirrors the C# reader loop, which exits the process on any
        // unexpected failure rather than leaving the extension half-alive.
        ExitProcess(1);
    }
}

void Processor::HandleResponse(const Response& response) {
    std::promise<Response> promise;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        auto it = pending_.find(response.request_id());
        if (it != pending_.end()) {
            promise = std::move(it->second);
            pending_.erase(it);
            found = true;
        }
    }
    if (found) {
        try {
            promise.set_value(response);
        } catch (const std::future_error&) {
            // Already abandoned (e.g. timed out) - ignore, mirrors
            // TaskCompletionSource semantics being best-effort here too.
        }
    }
}

void Processor::HandleEvent(const Event& event) {
    switch (event.event_case()) {
        case Event::kVirtualChannelReadyEvent:
            try {
                readyPromise_.set_value();
            } catch (const std::future_error&) {
            }
            break;
        case Event::kVirtualChannelClosedEvent:
            try {
                closedPromise_.set_value();
            } catch (const std::future_error&) {
            }
            break;
        default:
            break;
    }
}

}  // namespace DcvSdk

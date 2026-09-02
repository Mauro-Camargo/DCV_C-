#include "VirtualChannel.h"

#include <stdexcept>

namespace DcvSdk {

VirtualChannel::VirtualChannel(HANDLE pipe) : pipe_(pipe) {}

VirtualChannel::VirtualChannel(VirtualChannel&& other) noexcept : pipe_(other.pipe_) {
    other.pipe_ = INVALID_HANDLE_VALUE;
}

VirtualChannel& VirtualChannel::operator=(VirtualChannel&& other) noexcept {
    if (this != &other) {
        if (pipe_ != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe_);
        }
        pipe_ = other.pipe_;
        other.pipe_ = INVALID_HANDLE_VALUE;
    }
    return *this;
}

VirtualChannel::~VirtualChannel() {
    if (pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_);
    }
}

VirtualChannel VirtualChannel::Connect(const std::string& relayPath, const std::string& authToken) {
    HANDLE pipe = INVALID_HANDLE_VALUE;

    while (true) {
        pipe = CreateFileA(relayPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) {
            break;
        }

        DWORD err = GetLastError();
        if (err != ERROR_PIPE_BUSY) {
            throw std::runtime_error("Failed to open relay pipe: " + std::to_string(err));
        }
        if (!WaitNamedPipeA(relayPath.c_str(), 10000)) {
            throw std::runtime_error("Timed out waiting for relay pipe");
        }
    }

    VirtualChannel channel(pipe);
    channel.Write(authToken);
    return channel;
}

std::vector<uint8_t> VirtualChannel::Read() {
    uint8_t buffer[4096];
    DWORD read = 0;
    if (!ReadFile(pipe_, buffer, sizeof buffer, &read, nullptr) || read == 0) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + read);
}

void VirtualChannel::Write(const std::vector<uint8_t>& data) {
    DWORD total = 0;
    while (total < data.size()) {
        DWORD written = 0;
        if (!WriteFile(pipe_, data.data() + total, static_cast<DWORD>(data.size()) - total, &written, nullptr)) {
            throw std::runtime_error("Failed to write to virtual channel");
        }
        total += written;
    }
    FlushFileBuffers(pipe_);
}

void VirtualChannel::Write(const std::string& data) {
    Write(std::vector<uint8_t>(data.begin(), data.end()));
}

}  // namespace DcvSdk

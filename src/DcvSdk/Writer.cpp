#include "Writer.h"

#include <stdexcept>

namespace DcvSdk {

Writer::Writer(HANDLE output) : output_(output) {}

bool Writer::WriteExact(const uint8_t* buffer, DWORD size) {
    DWORD total = 0;
    while (total < size) {
        DWORD written = 0;
        if (!WriteFile(output_, buffer + total, size - total, &written, nullptr)) {
            return false;
        }
        total += written;
    }
    return true;
}

void Writer::SendRequest(const dcv::extensions::Request& request) {
    dcv::extensions::ExtensionMessage msg;
    *msg.mutable_request() = request;

    std::string serialized;
    if (!msg.SerializeToString(&serialized)) {
        throw std::runtime_error("Failed to serialize ExtensionMessage");
    }
    uint32_t size = static_cast<uint32_t>(serialized.size());

    std::lock_guard<std::mutex> lock(mutex_);
    if (!WriteExact(reinterpret_cast<const uint8_t*>(&size), sizeof size)) {
        throw std::runtime_error("Failed to write message header to stdout");
    }
    if (!WriteExact(reinterpret_cast<const uint8_t*>(serialized.data()), size)) {
        throw std::runtime_error("Failed to write message body to stdout");
    }
    FlushFileBuffers(output_);
}

}  // namespace DcvSdk

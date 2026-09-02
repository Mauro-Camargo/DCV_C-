#include "Reader.h"

#include <stdexcept>
#include <vector>

namespace DcvSdk {

Reader::Reader(HANDLE input) : input_(input) {}

bool Reader::ReadExact(uint8_t* buffer, DWORD size) {
    DWORD total = 0;
    while (total < size) {
        DWORD read = 0;
        if (!ReadFile(input_, buffer + total, size - total, &read, nullptr)) {
            return false;
        }
        if (read == 0) {
            return false;
        }
        total += read;
    }
    return true;
}

dcv::extensions::DcvMessage Reader::ReceiveMessage() {
    uint32_t size = 0;
    if (!ReadExact(reinterpret_cast<uint8_t*>(&size), sizeof size)) {
        ExitProcess(0);
    }

    // Matches the 1 MiB sanity cap enforced by the C# reader.
    if (size == 0 || size > 1'048'576) {
        throw std::runtime_error("Invalid DcvMessage size");
    }

    std::vector<uint8_t> body(size);
    if (!ReadExact(body.data(), size)) {
        ExitProcess(0);
    }

    dcv::extensions::DcvMessage msg;
    if (!msg.ParseFromArray(body.data(), static_cast<int>(size))) {
        throw std::runtime_error("Could not parse DcvMessage from stdin");
    }
    return msg;
}

}  // namespace DcvSdk

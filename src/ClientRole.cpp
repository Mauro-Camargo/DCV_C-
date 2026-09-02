#include "ClientRole.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace DcvLdiExtension {

namespace {

// Sleeps for `total`, waking up early (in <=200ms increments) if `cancelled`
// becomes true - the equivalent of C#'s cancellable Task.Delay(30s, ct).
void SleepCancellable(std::chrono::milliseconds total, std::atomic<bool>& cancelled) {
    const auto step = std::chrono::milliseconds(200);
    auto remaining = total;
    while (remaining.count() > 0 && !cancelled) {
        auto chunk = remaining < step ? remaining : step;
        std::this_thread::sleep_for(chunk);
        remaining -= chunk;
    }
}

std::string FormatUtcNow() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", st.wYear, st.wMonth, st.wDay, st.wHour,
                  st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

}  // namespace

void ClientRole::GetPrimaryInterface(std::string& ipv4, std::string& adapterType) {
    ipv4 = "0.0.0.0";
    adapterType = "UNKNOWN";

    ULONG bufLen = 15000;
    std::vector<uint8_t> buffer(bufLen);
    auto* addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG ret = GetAdaptersAddresses(AF_INET, flags, nullptr, addresses, &bufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(bufLen);
        addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        ret = GetAdaptersAddresses(AF_INET, flags, nullptr, addresses, &bufLen);
    }
    if (ret != NO_ERROR) {
        return;
    }

    for (auto* adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) continue;
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        for (auto* ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
            if (ua->Address.lpSockaddr->sa_family != AF_INET) continue;

            char ipStr[INET_ADDRSTRLEN] = {};
            auto* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
            inet_ntop(AF_INET, &sin->sin_addr, ipStr, sizeof ipStr);
            ipv4 = ipStr;

            switch (adapter->IfType) {
                case IF_TYPE_ETHERNET_CSMACD:
                    adapterType = "WIRED";
                    break;
                case IF_TYPE_IEEE80211:
                    adapterType = "WIRELESS";
                    break;
                default:
                    adapterType = "OTHER";
                    break;
            }
            return;
        }
    }
}

LdiPayload ClientRole::CollectLdi() {
    LdiPayload payload;

    char hostname[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = sizeof hostname;
    if (GetComputerNameA(hostname, &size)) {
        payload.hostname = hostname;
    }

    GetPrimaryInterface(payload.ipv4, payload.adapterType);
    payload.lastUpdatedUtc = FormatUtcNow();
    return payload;
}

void ClientRole::Run(DcvSdk::VirtualChannel& channel, std::atomic<bool>& cancelled) {
    while (!cancelled) {
        LdiPayload payload = CollectLdi();
        channel.Write(payload.ToJson());
        SleepCancellable(std::chrono::seconds(30), cancelled);
    }
}

void ClientRole::RunStandalone(std::atomic<bool>& cancelled) {
    while (!cancelled) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

}  // namespace DcvLdiExtension

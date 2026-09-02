#include "ServerRole.h"

#include <windows.h>
#include <shlobj.h>

#include <chrono>
#include <fstream>
#include <string>
#include <thread>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace DcvLdiExtension {

namespace {

constexpr char kPipePath[] = R"(\\.\pipe\Amazon\DCV-LDI-Extension\default\ldi)";
constexpr char kNotAvailable[] = R"({"state":"NOT_AVAILABLE"})";

void LogPipeError(const std::string& message) {
    PWSTR localAppData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
        std::wstring dir = std::wstring(localAppData) + L"\\Amazon\\DCV-LDI-Extension\\logs";
        CoTaskMemFree(localAppData);
        SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);

        std::ofstream log(dir + L"\\pipe-error.log", std::ios::app);
        if (log) {
            log << message << "\n";
        }
    }
}

}  // namespace

std::mutex ServerRole::payloadMutex_;
std::vector<uint8_t> ServerRole::currentPayload_(kNotAvailable, kNotAvailable + sizeof(kNotAvailable) - 1);

void ServerRole::ServeLdiPipe(std::atomic<bool>& cancelled) {
    while (!cancelled) {
        HANDLE pipe = CreateNamedPipeA(kPipePath, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_WAIT,
                                        PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) {
            LogPipeError("CreateNamedPipe failed: " + std::to_string(GetLastError()));
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected) {
            std::vector<uint8_t> payload;
            {
                std::lock_guard<std::mutex> lock(payloadMutex_);
                payload = currentPayload_;
            }
            DWORD written = 0;
            WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr);
            FlushFileBuffers(pipe);
        } else {
            LogPipeError("ConnectNamedPipe failed: " + std::to_string(GetLastError()));
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

void ServerRole::Run(DcvSdk::VirtualChannel& channel, bool channelReady, std::atomic<bool>& cancelled) {
    // Start serving the LDI pipe immediately (with NOT_AVAILABLE until data arrives).
    std::thread pipeThread(ServeLdiPipe, std::ref(cancelled));

    if (channelReady) {
        // Read data from the virtual channel (sent by the client extension).
        while (!cancelled) {
            std::vector<uint8_t> data = channel.Read();
            if (data.empty()) break;
            std::lock_guard<std::mutex> lock(payloadMutex_);
            currentPayload_ = std::move(data);
        }
    } else {
        // Channel not ready (client didn't connect the relay) - keep the pipe alive.
        while (!cancelled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    // Whatever the reason we stopped (cancellation or the channel going
    // away), make sure the pipe-serving loop observes it too.
    cancelled = true;

    // ServeLdiPipe may still be blocked in ConnectNamedPipe waiting for a
    // client; there's no clean way to interrupt that synchronously, so we
    // detach rather than join (the process is exiting shortly after this
    // call returns anyway).
    pipeThread.detach();
}

}  // namespace DcvLdiExtension

# DCV Local Device Info Extension (C++ port)

C++ port of the `.NET dcv-ldi-extension` project (see the sibling `DCV/`
folder), built following the patterns from AWS's own C++ sample
(`aws-samples/dcv-extension-sdk-samples`, `examples/cpp/extension-virtual-channel-cpp`):
Win32 APIs for stdio/named-pipe IO, synchronous reads/writes, and protobuf
for the DCV extension wire protocol.

Same functionality as the C# version: a single binary that runs on both the
DCV server and client sides, detects its role at runtime, and reports the
end user's hostname, session-facing IPv4 address, and network adapter type
(WIRED / WIRELESS / OTHER / UNKNOWN) to the server side over a named pipe.

## Repository layout

```
DCV_C++/
├── CMakeLists.txt              ← CMake + vcpkg (manifest mode) build
├── vcpkg.json                  ← declares the protobuf dependency
├── proto/
│   └── extensions.proto        ← same DCV Extension SDK schema as the C# project
├── src/
│   ├── main.cpp                ← entry point, role detection
│   ├── ClientRole.cpp/.h       ← collects LDI, sends over the channel
│   ├── ServerRole.cpp/.h       ← receives LDI, serves the named pipe
│   ├── LdiPayload.cpp/.h       ← hand-rolled JSON serialization
│   └── DcvSdk/
│       ├── Processor.cpp/.h    ← request/response + event dispatch
│       ├── Reader.cpp/.h       ← 4-byte LE header + protobuf reader
│       ├── Writer.cpp/.h       ← 4-byte LE header + protobuf writer
│       └── VirtualChannel.cpp/.h ← named-pipe relay connection
├── manifest/
│   └── dcv_extension_manifest.json
├── codesign/                   ← public cert, shared with the .NET project
└── .github/workflows/
    └── build-and-sign.yml      ← CMake+vcpkg build, then sign
```

## Differences from the C# version (and why)

| Area | C# | C++ port | Reason |
|---|---|---|---|
| Concurrency | `async`/`await`, `TaskCompletionSource` | Background thread + `std::promise`/`std::future` | No async runtime in plain Win32 C++; matches the "simple synchronous IO" style of AWS's own C++ sample |
| JSON | `System.Text.Json` (source-generated) | Hand-rolled serialization in `LdiPayload::ToJson()` | The payload is a small, fixed, flat schema — not worth a JSON library dependency. `FromJson` was unused dead code in the C# version (the server never parses the payload, just forwards raw bytes to the pipe) and isn't ported |
| Cancellation | `CancellationToken` threaded everywhere | An `std::atomic<bool>` flag, checked between blocking calls | Win32 `ReadFile` on stdin/pipes can't be cleanly interrupted without overlapped IO; the process is normally torn down by DCV/the OS anyway, matching how the C# reader also hard-exits (`Environment.Exit`) on EOF rather than a graceful join |
| Request IDs | `Guid.NewGuid()` | Incrementing counter | Uniqueness only needs to hold within one process's lifetime |

Functionally equivalent otherwise: same virtual channel namespace
(`com.amazon.dcv-ldi-extension`), same named pipe path
(`\\.\pipe\Amazon\DCV-LDI-Extension\default\ldi`), same JSON payload shape,
so a C++ client can talk to a C# server (and vice versa).

## Build

Requires Visual Studio 2022 (or the Build Tools) with the "Desktop
development with C++" workload, and git.

```powershell
git clone https://github.com/microsoft/vcpkg --depth 1
.\vcpkg\bootstrap-vcpkg.bat

cmake -S . -B build -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static

cmake --build build --config Release
```

The first configure will take a while — vcpkg builds protobuf from source.
Output: `build\Release\dcv-ldi-extension-cpp.exe` (statically linked, no
external DLL dependency, similar in spirit to the .NET project's
self-contained single-file publish).

## CI

`.github/workflows/build-and-sign.yml` runs the same build on
`windows-latest`, then signs the exe with the same self-signed certificate
used by the .NET project (see `codesign/README.md` for the secrets needed).

## Known limitations

Carried over from the .NET version:
- The WorkSpaces client does not provide stdin/stdout IPC to
  registry-launched 3rd-party extensions — the client-side binary runs in
  standalone mode in that case and cannot complete the DCV SDK handshake.
- Server pipe reports `{"state":"NOT_AVAILABLE"}` until client transport is
  resolved.

# Wirelink cross-library benchmark

This repository compares implementations of the same bounded application task.
It is separate from Wirelink's own performance regression benchmarks. The first
milestone is a reproducible source build and schema generation; no performance
claim is made until the adapters and hardware runner are implemented.

## First-round scope

- **RPC:** one `Setpoint` request and response. The client supplies a request
  ID, channel, and value. The server returns the same ID, a status, and the
  applied value. One call is complete only after the response is validated.
- **Telemetry:** a one-way `Sample` message with sequence, sample tick, value,
  and flags. The sender does not wait for an application reply.

The exact field ranges, expected behavior, and future timing boundaries are in
[workloads/v1.md](workloads/v1.md). Equivalent business meaning is required;
the libraries' wire formats and generated APIs are intentionally their own.

| Implementation | First-round role | Status |
| --- | --- | --- |
| Wirelink + WLC | full link, codec, managed RPC | local source and WLC codegen |
| eRPC | generated RPC and one-way method | upstream library and generator build |
| libcsp | transport and request/reply with a documented manual codec | upstream library build; adapter pending |
| Nanopb | payload codec | upstream library and generated C build |
| FlatCC | payload codec | upstream runtime/compiler and generated C build |

Nanopb and FlatCC are not complete RPC stacks. The libcsp adapter's manual
business codec will be included in future footprint and CPU measurements. eRPC
uses C++ infrastructure even when its generated API is C. Do not merge results
from these distinct layers into a single ranking.

## Reproduce the build milestone

On Linux, install CMake 3.21+, GCC or Clang C/C++, Python 3, Flex, Bison,
`protoc`, and Python `protobuf` and `kconfiglib` modules. Initialize the pinned
submodules, then run:

```sh
git submodule update --init --recursive
WIRELINK_SOURCE_DIR=/absolute/path/to/wirelink \
WLC_EXECUTABLE=/absolute/path/to/wlc \
  ./scripts/build-all.sh
```

`WIRELINK_SOURCE_DIR` defaults to `../wirelink`. `WLC_EXECUTABLE` defaults to
`wlc` on `PATH`. Generated files and builds go under ignored `build/`; the
script checks each pinned submodule commit. It builds the four upstream
libraries, generates all four schemas, and compiles the generated C sources
where applicable. It also compiles the local Wirelink core and WLC-generated
codec/runtime. It does not run performance measurements.

Dependency revisions and build assumptions are in [DEPENDENCIES.md](DEPENDENCIES.md).
The reference board for the later hardware stage is ESP32-S3 DevKitC. Hardware
results need two peers on the same physical link and a fixed toolchain/profile;
`native_sim` or host loopback timing will not be presented as MCU timing.

# Wirelink cross-library benchmark

This repository compares implementations of the same bounded application task.
It is separate from Wirelink's own performance regression benchmarks. Host
adapters now verify the task end to end; no performance claim is made until
the hardware runner and measurement protocol are implemented.

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
| Wirelink + WLC | full link, codec, managed RPC | host pair and generated codec smoke |
| eRPC | generated RPC and one-way method | two-process TCP smoke |
| libcsp | transport and request/reply with a documented manual codec | loopback pair and codec smoke |
| Nanopb | payload codec | generated codec smoke |
| FlatCC | payload codec | generated codec smoke |

Nanopb and FlatCC are not complete RPC stacks. The libcsp adapter's manual
business codec will be included in future footprint and CPU measurements. eRPC
uses C++ infrastructure even when its generated API is C. Do not merge results
from these distinct layers into a single ranking.

The generated codecs are used directly through small adapters. No RPC layer is
constructed around Nanopb or FlatCC just to create a comparison row. The
FlatCC table schema represents these scalar fields with defaults, whereas the
WLC and Nanopb schemas mark them required; first-round codec measurements are
therefore restricted to valid, complete messages. Decoder rejection behavior
is not considered equivalent across these schemas.

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
codec/runtime. Then verify adapters and business results:

```sh
WIRELINK_SOURCE_DIR=/absolute/path/to/wirelink ./scripts/smoke.sh
```

The Wirelink pair uses its generated managed endpoint and in-memory link. The
eRPC pair uses its generated client/server over localhost TCP. The libcsp pair
uses a persistent connection over CSP loopback; v2.1 callback routing requires
CRC32, so the smoke selects it. These three transport setups are correctness
checks, not comparable latency measurements. Nanopb and FlatCC only exercise
their generated payload codecs. The only hand-written payload serialization is
the necessary 12/16-byte big-endian libcsp business layout in
`adapters/libcsp_codec.c`; its cost must be included in later comparisons.
The FlatCC adapter currently initializes/finalizes a builder per message and
uses its allocation API. Generated eRPC shims also call `erpc_malloc` for
request and response structs. These allocations are part of the measured path
if these adapters are used for timing; a later optimized configuration must be
recorded as a separate variant.

Dependency revisions and build assumptions are in [DEPENDENCIES.md](DEPENDENCIES.md).
The reference board for the later hardware stage is ESP32-S3 DevKitC. Hardware
results need two peers on the same physical link and a fixed toolchain/profile;
`native_sim` or host loopback timing will not be presented as MCU timing.

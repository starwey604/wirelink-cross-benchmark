# Pinned source revisions

Git submodule gitlinks are the source of truth. `scripts/build-all.sh` verifies
these commits before building, so a local checkout cannot silently change the
measured implementation.

| Dependency | Revision | Selection |
| --- | --- | --- |
| eRPC | `8de773bf0f6f0d0c75da82b4be1f5ff9976997c3` | main snapshot with CMake/Kconfig support; the latest listed `1.9.1` tag predates that build path |
| libcsp | `48f7fb0b57f610bf65bab1aa2d1357c3b9722782` | v2.1 |
| Nanopb | `160d4f09e5fabb2b66aa2dea32d4f38ace2c4b3f` | nanopb-0.4.9.2 |
| FlatCC | `fec0275c0d0c5b6f495ab48d51fcde521d72b5fc` | v0.6.4 |

Wirelink is currently supplied by `WIRELINK_SOURCE_DIR`, because the local
working revision may not yet exist on the public remote. Before publishing
measurements, record its full git commit, dirty status, WLC version/contract,
compiler, flags, board revision, and transport configuration in the run
manifest. Do not compare a dirty source tree without preserving a source patch.

Build checks use native Release builds. The ESP32-S3 build and transport
adapters are a separate next milestone; these host artifacts are not MCU
footprint measurements.

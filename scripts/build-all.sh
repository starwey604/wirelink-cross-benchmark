#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$repo_root/build"
wirelink_source="${WIRELINK_SOURCE_DIR:-$repo_root/../wirelink}"
wlc_executable="${WLC_EXECUTABLE:-wlc}"
jobs="${JOBS:-4}"

require_revision() {
    local name="$1" expected="$2" actual
    if [[ ! -d "$repo_root/third_party/$name" ]]; then
        echo "missing third_party/$name; run git submodule update --init --recursive" >&2
        exit 2
    fi
    actual="$(git -C "$repo_root/third_party/$name" rev-parse HEAD)"
    if [[ "$actual" != "$expected" ]]; then
        echo "third_party/$name is $actual; expected $expected" >&2
        exit 2
    fi
}

require_revision erpc 8de773bf0f6f0d0c75da82b4be1f5ff9976997c3
require_revision libcsp 48f7fb0b57f610bf65bab1aa2d1357c3b9722782
require_revision nanopb 160d4f09e5fabb2b66aa2dea32d4f38ace2c4b3f
require_revision flatcc fec0275c0d0c5b6f495ab48d51fcde521d72b5fc

if [[ ! -f "$wirelink_source/CMakeLists.txt" ]]; then
    echo "WIRELINK_SOURCE_DIR must point to a Wirelink checkout" >&2
    exit 2
fi
if ! command -v "$wlc_executable" >/dev/null 2>&1; then
    echo "WLC_EXECUTABLE not found: $wlc_executable" >&2
    exit 2
fi

mkdir -p "$build_root/generated/erpc/objects" \
    "$build_root/generated/nanopb" \
    "$build_root/generated/flatcc" \
    "$build_root/generated/wirelink" \
    "$build_root/probes"

cmake -S "$repo_root/third_party/erpc" -B "$build_root/erpc" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_root/erpc" --target erpc erpcgen --parallel "$jobs"

cmake -S "$repo_root/third_party/libcsp" -B "$build_root/libcsp" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DCSP_BUILD_SAMPLES=OFF -DCSP_ENABLE_PYTHON3_BINDINGS=OFF
cmake --build "$build_root/libcsp" --target csp --parallel "$jobs"

cmake -S "$repo_root/third_party/nanopb" -B "$build_root/nanopb" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -Dnanopb_BUILD_GENERATOR=OFF -Dnanopb_BUILD_RUNTIME=ON
cmake --build "$build_root/nanopb" --target protobuf-nanopb-static --parallel "$jobs"

cmake -S "$repo_root/third_party/flatcc" -B "$build_root/flatcc" \
    -DCMAKE_BUILD_TYPE=Release -DFLATCC_TEST=OFF -DFLATCC_ALLOW_WERROR=OFF
cmake --build "$build_root/flatcc" --parallel "$jobs"

cmake -S "$wirelink_source" -B "$build_root/wirelink" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
    -DWIRELINK_BUILD_EXAMPLES=OFF -DWIRELINK_INSTALL=OFF
cmake --build "$build_root/wirelink" --target wirelink --parallel "$jobs"

"$build_root/erpc/erpcgen/erpcgen" -g c \
    -o "$build_root/generated/erpc" "$repo_root/workloads/erpc/task.erpc"
for source in "$build_root"/generated/erpc/*.cpp; do
    object="$build_root/generated/erpc/objects/$(basename "${source%.cpp}").o"
    "${CXX:-c++}" -std=c++11 -O2 \
        -I"$build_root/generated/erpc" \
        -I"$repo_root/third_party/erpc/erpc_c/config" \
        -I"$repo_root/third_party/erpc/erpc_c/infra" \
        -I"$repo_root/third_party/erpc/erpc_c/port" \
        -I"$repo_root/third_party/erpc/erpc_c/service" \
        -I"$repo_root/third_party/erpc/erpc_c/setup" \
        -I"$repo_root/third_party/erpc/erpc_c/transports" \
        -c "$source" -o "$object"
done

"${CC:-cc}" -std=c11 -O2 \
    -I"$repo_root/third_party/libcsp/include" \
    -I"$build_root/libcsp/include" \
    -c "$repo_root/probes/libcsp.c" -o "$build_root/probes/libcsp.o"

python3 "$repo_root/third_party/nanopb/generator/nanopb_generator.py" \
    -I "$repo_root/workloads/nanopb" \
    -D "$build_root/generated/nanopb" "$repo_root/workloads/nanopb/task.proto"
"${CC:-cc}" -std=c11 -O2 \
    -I"$repo_root/third_party/nanopb" \
    -I"$build_root/generated/nanopb" \
    -c "$build_root/generated/nanopb/task.pb.c" \
    -o "$build_root/probes/nanopb-generated.o"

"$repo_root/third_party/flatcc/bin/flatcc" -a \
    -o"$build_root/generated/flatcc" "$repo_root/workloads/flatcc/task.fbs"
"${CC:-cc}" -std=c11 -O2 \
    -I"$repo_root/third_party/flatcc/include" \
    -I"$build_root/generated/flatcc" \
    -c "$repo_root/probes/flatcc.c" -o "$build_root/probes/flatcc.o"

"$wlc_executable" compile "$repo_root/workloads/wirelink/task.wl" \
    --profile "$repo_root/workloads/wirelink/task.bind.wl" \
    --out-dir "$build_root/generated/wirelink"
for source in "$build_root"/generated/wirelink/*.c; do
    object="$build_root/generated/wirelink/$(basename "${source%.c}").o"
    "${CC:-cc}" -std=c11 -O2 \
        -I"$wirelink_source/include" \
        -I"$build_root/generated/wirelink" \
        -c "$source" -o "$object"
done

echo "All first-round libraries and generated workload sources compiled."

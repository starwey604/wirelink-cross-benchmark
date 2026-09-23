#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$repo_root/build"
wirelink_source="${WIRELINK_SOURCE_DIR:-$repo_root/../wirelink}"
cc="${CC:-cc}"
cxx="${CXX:-c++}"

if [[ ! -f "$build_root/wirelink/libwirelink.a" ||
      ! -f "$build_root/erpc/erpc_c/liberpc.a" ||
      ! -f "$build_root/libcsp/libcsp.a" ]]; then
    echo "Run scripts/build-all.sh first." >&2
    exit 2
fi
mkdir -p "$build_root/smoke"

"$cc" -std=c11 -O2 -I"$repo_root/adapters" \
    -I"$build_root/generated/wirelink" -I"$wirelink_source/include" \
    "$repo_root/adapters/codec_smoke.c" \
    "$repo_root/adapters/wirelink_codec.c" \
    "$build_root/generated/wirelink/task.c" \
    "$build_root/wirelink/libwirelink.a" -o "$build_root/smoke/wirelink-codec"
"$build_root/smoke/wirelink-codec"

"$cc" -std=c11 -O2 -I"$repo_root/adapters" \
    -I"$build_root/generated/nanopb" -I"$repo_root/third_party/nanopb" \
    "$repo_root/adapters/codec_smoke.c" \
    "$repo_root/adapters/nanopb_codec.c" \
    "$build_root/generated/nanopb/task.pb.c" \
    "$build_root/nanopb/libprotobuf-nanopb.a" \
    -o "$build_root/smoke/nanopb-codec"
"$build_root/smoke/nanopb-codec"

"$cc" -std=c11 -O2 -I"$repo_root/adapters" \
    -I"$build_root/generated/flatcc" -I"$repo_root/third_party/flatcc/include" \
    "$repo_root/adapters/codec_smoke.c" \
    "$repo_root/adapters/flatcc_codec.c" \
    "$repo_root/third_party/flatcc/lib/libflatccrt.a" \
    -o "$build_root/smoke/flatcc-codec"
"$build_root/smoke/flatcc-codec"

"$cc" -std=c11 -O2 -I"$repo_root/adapters" \
    "$repo_root/adapters/codec_smoke.c" \
    "$repo_root/adapters/libcsp_codec.c" -o "$build_root/smoke/libcsp-codec"
"$build_root/smoke/libcsp-codec"

cmake --build "$build_root/wirelink" --target wirelink_loopback
"$cc" -std=c11 -O2 \
    -I"$build_root/generated/wirelink" -I"$wirelink_source/include" \
    "$repo_root/adapters/wirelink_pair.c" \
    "$build_root/generated/wirelink/task.c" \
    "$build_root/generated/wirelink/task_bindings.c" \
    "$build_root/generated/wirelink/task_runtime.c" \
    "$build_root/wirelink/adapters/loopback/libwirelink_loopback.a" \
    "$build_root/wirelink/libwirelink.a" -o "$build_root/smoke/wirelink-pair"
"$build_root/smoke/wirelink-pair"

"$cc" -std=c11 -O2 -D_POSIX_C_SOURCE=200809L \
    -I"$repo_root/adapters" -I"$repo_root/third_party/libcsp/include" \
    -I"$build_root/libcsp/include" \
    "$repo_root/adapters/libcsp_pair.c" "$repo_root/adapters/libcsp_codec.c" \
    "$build_root/libcsp/libcsp.a" -pthread -o "$build_root/smoke/libcsp-pair"
timeout 10s "$build_root/smoke/libcsp-pair"

erpc_includes=(
    -I"$build_root/generated/erpc"
    -I"$repo_root/third_party/erpc/erpc_c/config"
    -I"$repo_root/third_party/erpc/erpc_c/infra"
    -I"$repo_root/third_party/erpc/erpc_c/port"
    -I"$repo_root/third_party/erpc/erpc_c/setup"
)
"$cc" -std=c11 -O2 "${erpc_includes[@]}" \
    -c "$repo_root/adapters/erpc_server.c" -o "$build_root/smoke/erpc-server-main.o"
"$cc" -std=c11 -O2 "${erpc_includes[@]}" \
    -c "$repo_root/adapters/erpc_client.c" -o "$build_root/smoke/erpc-client-main.o"
"$cxx" "$build_root/smoke/erpc-server-main.o" \
    "$build_root/generated/erpc/objects/c_wirelink_cross_benchmark_server.o" \
    "$build_root/generated/erpc/objects/wirelink_cross_benchmark_server.o" \
    "$build_root/generated/erpc/objects/wirelink_cross_benchmark_interface.o" \
    "$build_root/erpc/erpc_c/liberpc.a" -pthread -o "$build_root/smoke/erpc-server"
"$cxx" "$build_root/smoke/erpc-client-main.o" \
    "$build_root/generated/erpc/objects/c_wirelink_cross_benchmark_client.o" \
    "$build_root/generated/erpc/objects/wirelink_cross_benchmark_client.o" \
    "$build_root/generated/erpc/objects/wirelink_cross_benchmark_interface.o" \
    "$build_root/erpc/erpc_c/liberpc.a" -pthread -o "$build_root/smoke/erpc-client"

port="$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1]); s.close()')"
timeout 10s "$build_root/smoke/erpc-server" "$port" &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT
sleep 0.2
"$build_root/smoke/erpc-client" "$port"
wait "$server_pid"
trap - EXIT

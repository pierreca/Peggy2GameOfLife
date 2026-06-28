#!/usr/bin/env bash
# Host test runner for the toroidal edge-wrap logic (issue #4).
#
# Compiles Peggy2ConwayEngine against the shared host harness (HostGrid, selected
# via -DCONWAY_HOST_BUILD; issue #5) and runs the neighbour-count assertions on
# the host. The AddressSanitizer build additionally catches the out-of-bounds
# framebuffer read that the original `== -1` wrap produced off-hardware.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$HERE")"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

clang++ -std=c++14 -fsanitize=address -g -DCONWAY_HOST_BUILD \
  -I"$ROOT" -I"$ROOT/host" \
  "$HERE/test_edge_wrap.cpp" "$ROOT/Peggy2ConwayEngine.cpp" \
  -o "$OUT/test_edge_wrap"

"$OUT/test_edge_wrap"

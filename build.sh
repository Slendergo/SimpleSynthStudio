#!/usr/bin/env bash
set -e

config="${1:-Release}"
root="$(cd "$(dirname "$0")" && pwd)"

cmake -S "$root" -B "$root/build" -DCMAKE_BUILD_TYPE="$config" $CMAKE_ARGS
cmake --build "$root/build" --config "$config" --parallel

echo
echo "Built: $root/build/SimpleSynthStudio"
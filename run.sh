#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

export http_proxy="${http_proxy:-http://127.0.0.1:7897}"
export https_proxy="${https_proxy:-http://127.0.0.1:7897}"
export ALL_PROXY="${ALL_PROXY:-http://127.0.0.1:7897}"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/view_image "${1:-assets/sample.png}"

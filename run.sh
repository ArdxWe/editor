#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

cmake -S . -B build
cmake --build build
./build/view_image "${1:-assets/sample.png}"

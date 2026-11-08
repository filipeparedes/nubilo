#!/usr/bin/env bash
# Configures and builds the server. The tests are built by scripts/test.sh.
#
# Usage: scripts/build.sh [Debug|Release]    (Debug by default)
set -euo pipefail

cd "$(dirname "$0")/.."

cmake -S . -B build -DCMAKE_BUILD_TYPE="${1:-Debug}" -DFETCHCONTENT_QUIET=OFF
cmake --build build --target nubilo --parallel "${JOBS:-4}"
#!/usr/bin/env bash
# Starts the server, building it first if it was not built yet. It runs from
# the root of the repository, so nubilo.db and blobs/ are created there.
#
# Usage: scripts/run.sh [config file]    (config.json by default)
set -euo pipefail

cd "$(dirname "$0")/.."

if [ ! -x build/nubilo ]; then
  scripts/build.sh
fi

exec ./build/nubilo "$@"
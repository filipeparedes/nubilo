#!/usr/bin/env bash
# Builds the tests and runs them.
#
# Usage: scripts/test.sh                  all tests, through CTest
#        scripts/test.sh 'UtFilesStore.*'  only the tests that match a GoogleTest filter
#        scripts/test.sh 'Ut*'             unit tests only
#        scripts/test.sh 'It*'             integration tests only
set -euo pipefail

cd "$(dirname "$0")/.."

if [ ! -d build ]; then
  scripts/build.sh
fi
cmake --build build --target nubilo_tests --parallel "${JOBS:-4}"

if [ $# -eq 0 ]; then
  ctest --test-dir build --output-on-failure
else
  ./build/tests/nubilo_tests --gtest_filter="$1"
fi
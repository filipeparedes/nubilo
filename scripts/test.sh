#!/usr/bin/env bash
# Builds what the tests need and runs them: the unit tests (C++, GoogleTest)
# and the integration tests (Python, pytest).
#
# The integration tests need Python 3 with the packages of
# tests/integration/requirements.txt (pip install -r tests/integration/requirements.txt).
# They start the server binary, build/nubilo.
#
# Usage: scripts/test.sh                       unit and integration tests
#        scripts/test.sh unit                  unit tests, through CTest
#        scripts/test.sh unit 'UtFilesStore.*' only the unit tests that match a GoogleTest filter
#        scripts/test.sh integration           integration tests
#        scripts/test.sh integration -k cors   integration tests with pytest arguments
set -euo pipefail

cd "$(dirname "$0")/.."

suite="all"
if [ $# -gt 0 ]; then
  suite="$1"
  shift
fi

# Builds one target, configuring the project first if it was never built.
build() {
  if [ ! -d build ]; then
    scripts/build.sh
  fi
  cmake --build build --target "$1" --parallel "${JOBS:-4}"
}

run_unit() {
  build nubilo_tests
  if [ $# -eq 0 ]; then
    ctest --test-dir build --output-on-failure
  else
    ./build/tests/nubilo_tests --gtest_filter="$1"
  fi
}

run_integration() {
  build nubilo
  if ! python3 -c "import pytest, requests" 2>/dev/null; then
    echo "Install the Python packages first: pip install -r tests/integration/requirements.txt" >&2
    exit 1
  fi
  python3 -m pytest tests/integration "$@"
}

case "$suite" in
  all)
    run_unit
    run_integration
    ;;
  unit)
    run_unit "$@"
    ;;
  integration)
    run_integration "$@"
    ;;
  *)
    echo "Unknown suite '$suite'. Use: all, unit or integration." >&2
    exit 1
    ;;
esac

#!/bin/bash
# ============================================================
# GeoPoint CI Test Runner - Linux/macOS Shell Wrapper
# ============================================================
# Usage:
#   ./run_tests.sh                  - Debug build, run all tests
#   ./run_tests.sh Release          - Release build
#   ./run_tests.sh Debug --asan     - With AddressSanitizer
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="${1:-Debug}"

echo ""
echo "============================================================"
echo "  GeoPoint CI Test Suite - Linux/macOS"
echo "  Configuration: $CONFIG"
echo "============================================================"
echo ""

python3 "$SCRIPT_DIR/run_tests.py" --config "$CONFIG" "${@:2}"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo ""
    echo "[OK] All validations passed."
else
    echo ""
    echo "[ERROR] One or more validations failed. See test_report.html"
fi

exit $EXIT_CODE

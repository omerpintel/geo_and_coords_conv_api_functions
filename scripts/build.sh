#!/bin/bash
set -e

CONFIG="${1}"

if [ -z "$CONFIG" ]; then
    echo "Usage: ./build.sh [debug|release]"
    exit 1
fi

case "$CONFIG" in
    debug)
        PRESET="linux-debug"
        ;;
    release)
        PRESET="linux-release"
        ;;
    *)
        echo "Invalid configuration: $CONFIG"
        echo "Usage: ./build.sh [debug|release]"
        exit 1
        ;;
esac

# Navigate to project root (parent of scripts/)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "=== Configuring $CONFIG ==="
cmake --preset "$PRESET"

echo "=== Building $CONFIG ==="
cmake --build --preset "$PRESET"

echo "=== Build complete: $CONFIG ==="

"""
Package delivery artifacts into a 'delivery/' folder at project root.

Usage:
    python package.py                          # Auto-detect from default preset build dirs
    python package.py --build-dir out/build/windows-release
    python package.py --preset windows-release
"""

import os
import sys
import shutil
import platform
import argparse
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR

HEADERS = [
    PROJECT_ROOT / "include" / "api_functions.h",
    PROJECT_ROOT / "include" / "api_structs.h",
]

PRESET_BUILD_DIRS = {
    "windows-debug": "out/build/windows-debug",
    "windows-release": "out/build/windows-release",
    "windows-debug-vs": "out/build/windows-debug-vs",
    "linux-debug": "out/build/linux-debug",
    "linux-release": "out/build/linux-release",
    "linux-asan": "out/build/linux-asan",
    "ci": "out/build/ci",
}


def find_file_recursive(root: Path, name: str):
    """Find a file by name recursively under root."""
    for path in root.rglob(name):
        return path
    return None


def resolve_build_dir(args) -> Path:
    """Determine the build directory from args or auto-detect."""
    if args.build_dir:
        return Path(args.build_dir).resolve()

    if args.preset:
        rel = PRESET_BUILD_DIRS.get(args.preset)
        if not rel:
            print(f"[ERROR] Unknown preset '{args.preset}'. Available: {', '.join(PRESET_BUILD_DIRS.keys())}")
            sys.exit(1)
        return PROJECT_ROOT / rel

    # Auto-detect: try release first, then debug
    if platform.system() == "Windows":
        candidates = ["windows-release", "windows-debug", "windows-debug-vs", "ci"]
    else:
        candidates = ["linux-release", "linux-debug", "linux-asan", "ci"]

    for preset in candidates:
        d = PROJECT_ROOT / PRESET_BUILD_DIRS[preset]
        if d.exists():
            print(f"  Auto-detected build dir: {d}")
            return d

    print("[ERROR] No build directory found. Build the project first or specify --build-dir / --preset.")
    sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="Package delivery artifacts")
    parser.add_argument("--build-dir", help="Path to the CMake build directory")
    parser.add_argument("--preset", help=f"Preset name ({', '.join(PRESET_BUILD_DIRS.keys())})")
    parser.add_argument("--output", default="delivery", help="Output folder name (default: delivery)")
    args = parser.parse_args()

    build_dir = resolve_build_dir(args)
    output_dir = PROJECT_ROOT / args.output

    print("=" * 50)
    print("  PACKAGE DELIVERY ARTIFACTS")
    print("=" * 50)
    print(f"  Build dir:  {build_dir}")
    print(f"  Output dir: {output_dir}")
    print()

    if not build_dir.exists():
        print(f"[ERROR] Build directory does not exist: {build_dir}")
        sys.exit(1)

    # Determine platform-specific library names
    if platform.system() == "Windows":
        shared_lib_name = "api_functions.dll"
        import_lib_name = "api_functions.lib"
    elif platform.system() == "Darwin":
        shared_lib_name = "libapi_functions.dylib"
        import_lib_name = None
    else:
        shared_lib_name = "libapi_functions.so"
        import_lib_name = None

    # Find artifacts
    shared_lib = find_file_recursive(build_dir, shared_lib_name)
    import_lib = find_file_recursive(build_dir, import_lib_name) if import_lib_name else None

    if not shared_lib:
        print(f"[ERROR] Shared library '{shared_lib_name}' not found in {build_dir}")
        sys.exit(1)

    # Prepare output folder
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True)
    (output_dir / "include").mkdir()

    # Copy shared library
    shutil.copy2(shared_lib, output_dir / shared_lib_name)
    print(f"  [OK] {shared_lib_name}")

    # Copy import library (Windows only)
    if import_lib and import_lib.exists():
        shutil.copy2(import_lib, output_dir / import_lib_name)
        print(f"  [OK] {import_lib_name}")
    elif import_lib_name:
        print(f"  [WARN] Import library '{import_lib_name}' not found — skipping")

    # Copy headers
    for header in HEADERS:
        if header.exists():
            shutil.copy2(header, output_dir / "include" / header.name)
            print(f"  [OK] include/{header.name}")
        else:
            print(f"  [ERROR] Header not found: {header}")
            sys.exit(1)

    print()
    print(f"  Packaged to: {output_dir}")
    print("=" * 50)
    return 0


if __name__ == "__main__":
    sys.exit(main())

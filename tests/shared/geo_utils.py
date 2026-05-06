import ctypes
import os
import sys
import math
from enum import IntEnum

# --- 1. Shared C Structure ---
class SPointNE(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("north", ctypes.c_float), ("east", ctypes.c_float)]

    def __repr__(self):
        return f"SPointNE(N={self.north:.2f}, E={self.east:.2f})"

class EIsInsideResult(IntEnum):
    IS_INSIDE_OK = 0
    IS_INSIDE_POLYGON_IS_NULL_PTR = 1
    IS_INSIDE_POLYGON_WITH_LESS_THAN_3_POINTS = 2
    IS_INSIDE_OUTPUT_PTR_IS_NULL = 3

class ELineIntersectResult(IntEnum):
    LINE_INTERSECT_OK = 0
    LINE_INTERSECT_POLYGON_IS_NULL_PTR = 1
    LINE_INTERSECT_POLYGON_WITH_LESS_THAN_3_POINTS = 2
    LINE_INTERSECT_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO = 3
    LINE_INTERSECT_OUTPUT_PTR_IS_NULL = 4

# Legacy alias for visualization scripts
EResultState = EIsInsideResult

# --- 2. Shared Library Loader ---
def load_geopoint_library():
    """Finds and loads the api_functions DLL."""
    lib_name = "api_functions.dll" if sys.platform.startswith("win32") else "libapi_functions.so"
    
    # Go up two levels from 'tests/shared' to project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_dir)) 

    search_paths = [
        os.path.join(project_root, "out", "build", "WSL-GCC-Debug", "bin"),
        os.path.join(project_root, "out", "build", "x64-Debug", "bin"),
        os.path.join(project_root, "out", "build", "x64-Release", "bin"),
        os.path.join(project_root, "build", "bin"),
        os.path.join(project_root, "bin") # Common output dir
    ]

    for path in search_paths:
        lib_path = os.path.join(path, lib_name)
        if os.path.exists(lib_path):
            print(f"[INFO] Loaded library from: {lib_path}")
            if sys.platform.startswith("win32"):
                os.add_dll_directory(os.path.dirname(lib_path))
            return ctypes.CDLL(lib_path)

    print(f"[ERROR] Could not find '{lib_name}' in standard build folders.", file=sys.stderr)
    print(f"Searched in: {search_paths}", file=sys.stderr)
    sys.exit(1)

# --- 3. Shared Math Helper ---
def ned_to_geodetic(north, east, origin_lat, origin_lon):
    EARTH_RADIUS = 6371000.0
    lat = origin_lat + math.degrees(north / EARTH_RADIUS)
    lon = origin_lon + math.degrees(east / (EARTH_RADIUS * math.cos(math.radians(origin_lat))))
    return lat, lon
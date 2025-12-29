import ctypes
import os
import sys
import math
from enum import IntEnum

# --- 1. Shared C Structure ---
class Point(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("north", ctypes.c_float), ("east", ctypes.c_float)]

    def __repr__(self):
        return f"Point(N={self.north:.2f}, E={self.east:.2f})"

class EResultState(IntEnum):
    OK = 0
    POLYGON_WITH_LESS_THAN_3_POINTS = 1
    POLYGON_IS_NULL_PTR = 2
    MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO = 3

# --- 2. Shared Library Loader ---
def load_geopoint_library():
    """Finds and loads the api_functions DLL."""
    lib_name = "api_functions.dll" if sys.platform.startswith("win32") else "api_functions.so"
    
    # Go up one level from 'scripts' to project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir) 

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
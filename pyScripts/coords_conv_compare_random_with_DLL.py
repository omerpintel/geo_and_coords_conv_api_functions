import ctypes
import math
import random
import pymap3d as pm

import geo_utils

# --- Configuration ---
NUM_TESTS = 1000
LIB_PATH = "./libgeo_api.so"  # Change to .dll on Windows


# --- 1. Define C++ Structs ---
class SPointGeo(ctypes.Structure):
    _fields_ = [("latitudeDeg", ctypes.c_double),
                ("longitudeDeg", ctypes.c_double),
                ("altitude", ctypes.c_double)]


class SPointNED(ctypes.Structure):
    _fields_ = [("north", ctypes.c_double),
                ("east", ctypes.c_double),
                ("down", ctypes.c_double)]


# --- 2. Load Library ---
lib = geo_utils.load_geopoint_library()

# Set function signatures
lib.GeoToNed.argtypes = [ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(SPointGeo)]
lib.GeoToNed.restype = SPointNED

lib.NedToGeo.argtypes = [ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(SPointNED)]
lib.NedToGeo.restype = SPointGeo


# --- 3. Helper Functions ---
def get_random_geo(center_lat, center_lon, radius_deg=1.0):
    """Generates a random Geo point within roughly radius_deg of center."""
    lat = center_lat + random.uniform(-radius_deg, radius_deg)
    lon = center_lon + random.uniform(-radius_deg, radius_deg)
    alt = random.uniform(-100, 1000)  # Random altitude
    return SPointGeo(lat, lon, alt)


def get_random_ned(radius_m=5000.0):
    """Generates a random NED point within radius_m meters."""
    n = random.uniform(-radius_m, radius_m)
    e = random.uniform(-radius_m, radius_m)
    d = random.uniform(-500, 500)  # Vertical variation
    return SPointNED(n, e, d)


# --- 4. Test Logic ---
def run_stress_test():
    print(f"Starting Stress Test: {NUM_TESTS} random points...")

    # Validation thresholds
    TOLERANCE_POS_M = 0.05  # 5 cm tolerance for NED
    TOLERANCE_DEG = 1e-6  # ~11 cm tolerance for Lat/Lon

    max_error_ned = 0.0
    max_error_geo = 0.0
    failed_count = 0

    # Fixed Origin for all tests (can also be randomized if needed)
    lat0_val = 32.0
    lon0_val = 34.0
    c_lat0 = ctypes.c_double(lat0_val)
    c_lon0 = ctypes.c_double(lon0_val)

    for i in range(NUM_TESTS):
    #     # -------------------------------------------------
    #     # TEST 1: Geo -> NED
    #     # -------------------------------------------------
    #     input_geo = get_random_geo(lat0_val, lon0_val)
    #
    #     # Pymap3d (Truth) - Assuming origin altitude is 0
    #     pm_n, pm_e, pm_d = pm.geodetic2ned(
    #         input_geo.latitudeDeg, input_geo.longitudeDeg, input_geo.altitude,
    #         lat0_val, lon0_val, 0
    #     )
    #
    #     # C++ API (Test)
    #     res_ned = lib.GeoToNed(ctypes.byref(c_lat0), ctypes.byref(c_lon0), ctypes.byref(input_geo))
    #
    #     # Check Errors
    #     diff_n = abs(res_ned.north - pm_n)
    #     diff_e = abs(res_ned.east - pm_e)
    #     diff_d = abs(res_ned.down - pm_d)
    #     total_err = math.sqrt(diff_n ** 2 + diff_e ** 2 + diff_d ** 2)
    #
    #     max_error_ned = max(max_error_ned, total_err)
    #
    #     if total_err > TOLERANCE_POS_M:
    #         print(f"[FAIL] GeoToNed Iteration {i}")
    #         print(f"  Input: {input_geo.latitudeDeg}, {input_geo.longitudeDeg}")
    #         print(f"  PyMap3d: {pm_n:.3f}, {pm_e:.3f}, {pm_d:.3f}")
    #         print(f"  Cpp    : {res_ned.north:.3f}, {res_ned.east:.3f}, {res_ned.down:.3f}")
    #         failed_count += 1

        # -------------------------------------------------
        # TEST 2: NED -> Geo
        # -------------------------------------------------
        input_ned = get_random_ned()

        # Pymap3d (Truth)
        pm_lat, pm_lon, pm_alt = pm.ned2geodetic(
            input_ned.north, input_ned.east, input_ned.down,
            lat0_val, lon0_val, 0
        )

        # C++ API (Test)
        res_geo = lib.NedToGeo(ctypes.byref(c_lat0), ctypes.byref(c_lon0), ctypes.byref(input_ned))

        # Check Errors
        diff_lat = abs(res_geo.latitudeDeg - pm_lat)
        diff_lon = abs(res_geo.longitudeDeg - pm_lon)
        diff_alt = abs(res_geo.altitude - pm_alt)

        max_error_geo = max(max_error_geo, diff_lat, diff_lon, diff_alt)

        if diff_lat > TOLERANCE_DEG or diff_lon > TOLERANCE_DEG or diff_alt > TOLERANCE_DEG:
            print(f"[FAIL] NedToGeo Iteration {i}")
            print(f"  Input NED: {input_ned.north}, {input_ned.east}, {input_ned.down}")
            print(f"  PyMap3d: {pm_lat:.6f}, {pm_lon:.6f}, {pm_alt:.6f}")
            print(f"  Cpp    : {res_geo.latitudeDeg:.6f}, {res_geo.longitudeDeg:.6f}, {res_geo.altitude:.6f}")
            failed_count += 1

    # --- Summary ---
    print("-" * 40)
    print(f"Completed {NUM_TESTS} tests.")
    print(f"Max NED Error (Distance): {max_error_ned:.6f} meters")
    print(f"Max Geo Error (Degrees) : {max_error_geo:.9f} degrees")

    if failed_count == 0:
        print(">> ALL TESTS PASSED within tolerance.")
    else:
        print(f">> {failed_count} tests FAILED.")


if __name__ == "__main__":
    run_stress_test()
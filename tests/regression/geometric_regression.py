"""
Geometric Functions Regression Test Suite
==========================================
Tests isInsidePolygonNED, isInsidePolygonGeo, doesLineIntersectPolygonNED,
and doesLineIntersectPolygonGeo via the shared library (DLL/SO).

All isInsidePolygon tests are run with radius=0 to verify pure point-in-polygon logic.

Usage:
    python geometric_regression.py
    python geometric_regression.py --verbose

Exit code 0 = all passed, 1 = one or more failures.
"""

import sys
import os
import ctypes
import argparse

# Add shared utils to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'shared'))
from geo_utils import (
    load_geopoint_library, SPointNE, SPointGeo,
    EIsInsideResult, ELineIntersectResult,
    EIsInsideGeoResult, ELineIntersectGeoResult
)

# --- Load Library ---
lib = load_geopoint_library()

# --- Configure function signatures ---
lib.isInsidePolygonNED.argtypes = [
    ctypes.POINTER(SPointNE), ctypes.c_uint16, SPointNE,
    ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
]
lib.isInsidePolygonNED.restype = None

lib.doesLineIntersectPolygonNED.argtypes = [
    ctypes.POINTER(SPointNE), ctypes.c_uint16, SPointNE,
    ctypes.c_float, ctypes.c_float,
    ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
]
lib.doesLineIntersectPolygonNED.restype = None

lib.isInsidePolygonGeo.argtypes = [
    ctypes.POINTER(SPointGeo), ctypes.c_uint16, SPointGeo,
    ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
]
lib.isInsidePolygonGeo.restype = None

lib.doesLineIntersectPolygonGeo.argtypes = [
    ctypes.POINTER(SPointGeo), ctypes.c_uint16, SPointGeo,
    ctypes.c_float, ctypes.c_float,
    ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
]
lib.doesLineIntersectPolygonGeo.restype = None


# --- Helpers ---
def make_ned_poly(points):
    """Create a ctypes array of SPointNE from list of (north, east) tuples."""
    arr = (SPointNE * len(points))()
    for i, (n, e) in enumerate(points):
        arr[i] = SPointNE(n, e)
    return arr, len(points)


def make_geo_poly(points):
    """Create a ctypes array of SPointGeo from list of (lat, lon, alt) tuples."""
    arr = (SPointGeo * len(points))()
    for i, (lat, lon, alt) in enumerate(points):
        arr[i] = SPointGeo(lat, lon, alt)
    return arr, len(points)


def call_is_inside_ned(poly_arr, count, north, east, radius):
    """Call isInsidePolygonNED, return (result, state)."""
    res = ctypes.c_uint8(0)
    state = ctypes.c_uint8(0)
    pt = SPointNE(north, east)
    lib.isInsidePolygonNED(poly_arr, count, pt, ctypes.c_float(radius), ctypes.byref(res), ctypes.byref(state))
    return res.value, state.value


def call_is_inside_geo(poly_arr, count, lat, lon, alt, radius):
    """Call isInsidePolygonGeo, return (result, state)."""
    res = ctypes.c_uint8(0)
    state = ctypes.c_uint8(0)
    pt = SPointGeo(lat, lon, alt)
    lib.isInsidePolygonGeo(poly_arr, count, pt, ctypes.c_float(radius), ctypes.byref(res), ctypes.byref(state))
    return res.value, state.value


def call_line_intersect_ned(poly_arr, count, north, east, azimuth, length):
    """Call doesLineIntersectPolygonNED, return (result, state)."""
    res = ctypes.c_uint8(0)
    state = ctypes.c_uint8(0)
    pt = SPointNE(north, east)
    lib.doesLineIntersectPolygonNED(poly_arr, count, pt, ctypes.c_float(azimuth),
                                     ctypes.c_float(length), ctypes.byref(res), ctypes.byref(state))
    return res.value, state.value


def call_line_intersect_geo(poly_arr, count, lat, lon, alt, azimuth, length):
    """Call doesLineIntersectPolygonGeo, return (result, state)."""
    res = ctypes.c_uint8(0)
    state = ctypes.c_uint8(0)
    pt = SPointGeo(lat, lon, alt)
    lib.doesLineIntersectPolygonGeo(poly_arr, count, pt, ctypes.c_float(azimuth),
                                     ctypes.c_float(length), ctypes.byref(res), ctypes.byref(state))
    return res.value, state.value


# --- Test Data ---
SQUARE_NED = [(0, 0), (0, 10), (10, 10), (10, 0)]
U_SHAPE_NED = [(0, 0), (10, 0), (10, 10), (7, 10), (7, 3), (3, 3), (3, 10), (0, 10)]
TRIANGLE_NED = [(0, 0), (10, 2), (0, 4)]
L_SHAPE_NED = [(0, 0), (0, 10), (5, 10), (5, 5), (10, 5), (10, 0)]
PENTAGON_NED = [(5, 0), (1.5, 4.8), (3.1, 8), (6.9, 8), (8.5, 4.8)]

GEO_CENTER_LAT = 32.0853
GEO_CENTER_LON = 34.7818
GEO_HALF_LAT = 0.00225
GEO_HALF_LON = 0.0025

SQUARE_GEO = [
    (GEO_CENTER_LAT - GEO_HALF_LAT, GEO_CENTER_LON - GEO_HALF_LON, 0),
    (GEO_CENTER_LAT - GEO_HALF_LAT, GEO_CENTER_LON + GEO_HALF_LON, 0),
    (GEO_CENTER_LAT + GEO_HALF_LAT, GEO_CENTER_LON + GEO_HALF_LON, 0),
    (GEO_CENTER_LAT + GEO_HALF_LAT, GEO_CENTER_LON - GEO_HALF_LON, 0),
]
TRIANGLE_GEO = [
    (32.080, 34.775, 0), (32.090, 34.782, 0), (32.080, 34.789, 0)
]


# --- Test Cases ---
# Format: (name, polygon_key, north, east, radius, expected_collision)
IS_INSIDE_NED_R0_TESTS = [
    # Square polygon
    ("NED R0: Inside center", "square", 5, 5, 0, True),
    ("NED R0: Outside strict", "square", 20, 5, 0, False),
    ("NED R0: On boundary (south edge)", "square", 0, 5, 0, True),
    ("NED R0: Near boundary outside", "square", -1, 5, 0, False),
    ("NED R0: Far outside", "square", -5, 5, 0, False),
    ("NED R0: Far outside diagonal", "square", 20, 20, 0, False),
    ("NED R0: On east edge", "square", 5, 10, 0, True),
    ("NED R0: Exact vertex (0,0)", "square", 0, 0, 0, True),
    ("NED R0: Exact vertex (10,10)", "square", 10, 10, 0, True),
    ("NED R0: Outside near west edge", "square", -2, 5, 0, False),
    # U-shape
    ("NED R0: Concave bay (outside)", "u_shape", 5, 8, 0, False),
    # Triangle
    ("NED R0: Sharp vertex tip", "triangle", 10, 2, 0, True),
    ("NED R0: Off vertex (outside)", "triangle", 10.1, 2, 0, False),
    # L-shape
    ("NED R0: L-shape arm inside", "l_shape", 2, 8, 0, True),
    ("NED R0: L-shape base inside", "l_shape", 8, 2, 0, True),
    ("NED R0: L-shape notch outside", "l_shape", 8, 8, 0, False),
    # Pentagon
    ("NED R0: Pentagon inside", "pentagon", 5, 5, 0, True),
    ("NED R0: Pentagon outside", "pentagon", 0, 0, 0, False),
]

IS_INSIDE_GEO_R0_TESTS = [
    ("GEO R0: Inside center", "square", GEO_CENTER_LAT, GEO_CENTER_LON, 0, True),
    ("GEO R0: Outside far", "square", 32.10, 34.79, 0, False),
    ("GEO R0: Near north (outside)", "square", GEO_CENTER_LAT + GEO_HALF_LAT + 0.00045, GEO_CENTER_LON, 0, False),
    ("GEO R0: Far north (outside)", "square", GEO_CENTER_LAT + GEO_HALF_LAT + 0.0027, GEO_CENTER_LON, 0, False),
    ("GEO R0: Near south edge outside", "square", GEO_CENTER_LAT - GEO_HALF_LAT - 0.000027, GEO_CENTER_LON, 0, False),
    ("GEO R0: Triangle inside", "triangle", 32.084, 34.782, 0, True),
    ("GEO R0: Triangle outside", "triangle", 32.070, 34.770, 0, False),
    ("GEO R0: On vertex", "triangle", 32.080, 34.775, 0, True),
    ("GEO R0: On closing edge", "square", GEO_CENTER_LAT, GEO_CENTER_LON - GEO_HALF_LON, 0, True),
    ("GEO R0: On south edge", "square", GEO_CENTER_LAT - GEO_HALF_LAT, GEO_CENTER_LON, 0, True),
    ("GEO R0: On east edge", "square", GEO_CENTER_LAT, GEO_CENTER_LON + GEO_HALF_LON, 0, True),
]

LINE_INTERSECT_NED_TESTS = [
    # (name, polygon_key, north, east, azimuth, length, expected)
    ("NED Line: Inside out", "square", 5, 5, 0, 100, True),
    ("NED Line: Contained", "square", 5, 5, 0, 1, True),
    ("NED Line: Outside parallel", "square", -5, -0.1, 0, 10, False),
    ("NED Line: Crossing in", "square", 5, -5, 90, 20, True),
    ("NED Line: Too short", "square", 5, -5, 90, 4.9, False),
    ("NED Line: Reaches edge", "square", 5, -5, 90, 5, True),
    ("NED Line: Going away", "square", -5, 5, 180, 100, False),
]

LINE_INTERSECT_GEO_TESTS = [
    # (name, polygon_key, lat, lon, azimuth, length, expected)
    ("GEO Line: From inside", "square", GEO_CENTER_LAT, GEO_CENTER_LON, 0, 500, True),
    ("GEO Line: Crossing in", "square", GEO_CENTER_LAT, GEO_CENTER_LON - 0.01, 90, 2000, True),
    ("GEO Line: Away", "square", GEO_CENTER_LAT + 0.01, GEO_CENTER_LON, 0, 500, False),
    ("GEO Line: Too short", "square", GEO_CENTER_LAT, GEO_CENTER_LON - 0.01, 90, 100, False),
    ("GEO Line: Triangle hit", "triangle", 32.085, 34.770, 90, 2000, True),
    ("GEO Line: Triangle miss", "triangle", 32.095, 34.770, 90, 2000, False),
]


def run_tests(verbose=False):
    """Run all regression tests. Returns (passed, failed)."""
    passed = 0
    failed = 0

    ned_polys = {
        "square": make_ned_poly(SQUARE_NED),
        "u_shape": make_ned_poly(U_SHAPE_NED),
        "triangle": make_ned_poly(TRIANGLE_NED),
        "l_shape": make_ned_poly(L_SHAPE_NED),
        "pentagon": make_ned_poly(PENTAGON_NED),
    }

    geo_polys = {
        "square": make_geo_poly(SQUARE_GEO),
        "triangle": make_geo_poly(TRIANGLE_GEO),
    }

    # --- isInsidePolygonNED (radius=0) ---
    print("\n--- isInsidePolygonNED (radius=0 regression) ---")
    for name, poly_key, north, east, radius, expected in IS_INSIDE_NED_R0_TESTS:
        arr, count = ned_polys[poly_key]
        result, state = call_is_inside_ned(arr, count, north, east, radius)
        actual = (state == EIsInsideResult.IS_INSIDE_OK and result == 1)
        if actual == expected:
            passed += 1
            if verbose:
                print(f"  [PASS] {name}")
        else:
            failed += 1
            print(f"  [FAIL] {name} | expected={expected}, got={actual} (result={result}, state={state})")

    # --- isInsidePolygonGeo (radius=0) ---
    print("\n--- isInsidePolygonGeo (radius=0 regression) ---")
    for name, poly_key, lat, lon, radius, expected in IS_INSIDE_GEO_R0_TESTS:
        arr, count = geo_polys[poly_key]
        result, state = call_is_inside_geo(arr, count, lat, lon, 0.0, radius)
        actual = (state == EIsInsideGeoResult.IS_INSIDE_GEO_OK and result == 1)
        if actual == expected:
            passed += 1
            if verbose:
                print(f"  [PASS] {name}")
        else:
            failed += 1
            print(f"  [FAIL] {name} | expected={expected}, got={actual} (result={result}, state={state})")

    # --- doesLineIntersectPolygonNED ---
    print("\n--- doesLineIntersectPolygonNED (regression) ---")
    for name, poly_key, north, east, az, length, expected in LINE_INTERSECT_NED_TESTS:
        arr, count = ned_polys[poly_key]
        result, state = call_line_intersect_ned(arr, count, north, east, az, length)
        actual = (state == ELineIntersectResult.LINE_INTERSECT_OK and result == 1)
        if actual == expected:
            passed += 1
            if verbose:
                print(f"  [PASS] {name}")
        else:
            failed += 1
            print(f"  [FAIL] {name} | expected={expected}, got={actual} (result={result}, state={state})")

    # --- doesLineIntersectPolygonGeo ---
    print("\n--- doesLineIntersectPolygonGeo (regression) ---")
    for name, poly_key, lat, lon, az, length, expected in LINE_INTERSECT_GEO_TESTS:
        arr, count = geo_polys[poly_key]
        result, state = call_line_intersect_geo(arr, count, lat, lon, 0.0, az, length)
        actual = (state == ELineIntersectGeoResult.LINE_INTERSECT_GEO_OK and result == 1)
        if actual == expected:
            passed += 1
            if verbose:
                print(f"  [PASS] {name}")
        else:
            failed += 1
            print(f"  [FAIL] {name} | expected={expected}, got={actual} (result={result}, state={state})")

    return passed, failed


def main():
    parser = argparse.ArgumentParser(description="Geometric functions regression test")
    parser.add_argument("--verbose", "-v", action="store_true", help="Print all test results")
    args = parser.parse_args()

    print("=" * 60)
    print("  GEOMETRIC FUNCTIONS REGRESSION TEST")
    print("=" * 60)

    passed, failed = run_tests(verbose=args.verbose)

    print(f"\n{'=' * 60}")
    print(f"  RESULT: {passed} passed, {failed} failed")
    print(f"{'=' * 60}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

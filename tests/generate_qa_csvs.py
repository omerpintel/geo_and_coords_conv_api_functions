"""
QA CSV Generator
================
Generates QA validation CSV files by calling the actual compiled DLL.
Produces 6 CSV files in the qa/ directory:
  - QA_isInsidePolygon.csv         (NED)
  - QA_doesLineIntersectPolygon.csv (NED)
  - QA_isInsidePolygonGeo.csv      (GEO)
  - QA_doesLineIntersectPolygonGeo.csv (GEO)
  - QA_GeoToNed.csv
  - QA_NedToGeo.csv

Usage:
    python generate_qa_csvs.py
"""

import sys
import os
import ctypes
import csv

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'shared'))
from geo_utils import (
    load_geopoint_library, SPointNE, SPointGeo,
    EIsInsideResult, ELineIntersectResult,
    EIsInsideGeoResult, ELineIntersectGeoResult
)


# --- Structs ---
class SPointNED(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("north", ctypes.c_double),
                ("east", ctypes.c_double),
                ("down", ctypes.c_double)]


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

lib.GeoToNed.argtypes = [SPointGeo, SPointGeo, ctypes.POINTER(SPointNED)]
lib.GeoToNed.restype = None

lib.NedToGeo.argtypes = [SPointGeo, SPointNED, ctypes.POINTER(SPointGeo)]
lib.NedToGeo.restype = None


# --- Helpers ---
def make_ned_poly(points):
    arr = (SPointNE * len(points))()
    for i, (n, e) in enumerate(points):
        arr[i] = SPointNE(n, e)
    return arr, len(points)


def make_geo_poly(points):
    arr = (SPointGeo * len(points))()
    for i, (lat, lon, alt) in enumerate(points):
        arr[i] = SPointGeo(lat, lon, alt)
    return arr, len(points)


def format_ned_poly(points):
    """Format polygon as string like '(0,0) (0,10) (10,10) (10,0)'"""
    return " ".join(f"({n},{e})" for n, e in points)


def format_geo_poly(points):
    """Format geo polygon as string like '(32.08,34.78,0) (32.08,34.79,0)'"""
    return " ".join(f"({lat},{lon},{alt})" for lat, lon, alt in points)


def is_inside_result_name(state):
    names = {0: "OK", 1: "POLYGON_IS_NULL_PTR", 2: "POLYGON_WITH_LESS_THAN_3_POINTS",
             3: "OUTPUT_PTR_IS_NULL", 4: "POLYGON_EXCEEDS_MAX_VERTICES"}
    return names.get(state, f"UNKNOWN({state})")


def line_intersect_result_name(state):
    names = {0: "OK", 1: "POLYGON_IS_NULL_PTR", 2: "POLYGON_WITH_LESS_THAN_3_POINTS",
             3: "MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO", 4: "OUTPUT_PTR_IS_NULL",
             5: "POLYGON_EXCEEDS_MAX_VERTICES"}
    return names.get(state, f"UNKNOWN({state})")


# --- Test data ---
SQUARE_NED = [(0, 0), (0, 10), (10, 10), (10, 0)]
U_SHAPE_NED = [(0, 0), (10, 0), (10, 10), (7, 10), (7, 3), (3, 3), (3, 10), (0, 10)]
TRIANGLE_NED = [(0, 0), (10, 2), (0, 4)]

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


# =============================================================================
# QA_isInsidePolygon.csv (NED)
# =============================================================================
def generate_is_inside_polygon_ned(qa_dir):
    print("Generating QA_isInsidePolygon.csv ...")
    header = ["Description", "Polygon Vertices (North/East in meters)",
              "Test Point North (m)", "Test Point East (m)", "Radius (m)",
              "Expected Result", "Expected Error Code"]

    test_cases = [
        # (description, polygon_points_or_None, north, east, radius)
        ("Point at centre of 10x10m square", SQUARE_NED, 5, 5, 0),
        ("Point well outside polygon", SQUARE_NED, 20, 5, 0),
        ("Boundary counts as inside", SQUARE_NED, 0, 5, 0),
        ("Point 1m outside but 2m radius reaches edge", SQUARE_NED, -1, 5, 2),
        ("Point 5m outside and 2m radius does not reach", SQUARE_NED, -5, 5, 2),
        ("Exactly on the east edge", SQUARE_NED, 5, 10, 0),
        ("Far from polygon bounding box", SQUARE_NED, 20, 20, 1),
        ("Point in empty bay of U-shape", U_SHAPE_NED, 5, 8, 0),
        ("2.1m radius hits inner walls of U-shape", U_SHAPE_NED, 5, 8, 2.1),
        ("1.9m radius fits in 2m gap of U-shape", U_SHAPE_NED, 5, 8, 1.9),
        ("Exact vertex of triangle counts as inside", TRIANGLE_NED, 10, 2, 0),
        ("Point just off tip but radius catches it", TRIANGLE_NED, 10.1, 2, 0.2),
        # Error cases
        ("Null polygon pointer", None, 5, 5, 0),
        ("Polygon with fewer than 3 points", [(0, 0), (0, 10)], 5, 5, 0),
    ]

    rows = []
    for desc, poly_pts, north, east, radius in test_cases:
        if poly_pts is None:
            # Null polygon test
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointNE(north, east)
            lib.isInsidePolygonNED(None, 0, pt, ctypes.c_float(radius),
                                   ctypes.byref(res), ctypes.byref(state))
            poly_str = "NULL"
        elif len(poly_pts) < 3:
            arr, count = make_ned_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointNE(north, east)
            lib.isInsidePolygonNED(arr, count, pt, ctypes.c_float(radius),
                                   ctypes.byref(res), ctypes.byref(state))
            poly_str = format_ned_poly(poly_pts)
        else:
            arr, count = make_ned_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointNE(north, east)
            lib.isInsidePolygonNED(arr, count, pt, ctypes.c_float(radius),
                                   ctypes.byref(res), ctypes.byref(state))
            poly_str = format_ned_poly(poly_pts)

        result_bool = "TRUE" if res.value == 1 else "FALSE"
        error_code = is_inside_result_name(state.value)
        rows.append([desc, poly_str, north, east, radius, result_bool, error_code])

    filepath = os.path.join(qa_dir, "QA_isInsidePolygon.csv")
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
    print(f"  Written {len(rows)} test cases to {filepath}")


# =============================================================================
# QA_doesLineIntersectPolygon.csv (NED)
# =============================================================================
def generate_does_line_intersect_ned(qa_dir):
    print("Generating QA_doesLineIntersectPolygon.csv ...")
    header = ["Description", "Polygon Vertices (North/East in meters)",
              "Start Point North (m)", "Start Point East (m)",
              "Azimuth (deg from North)", "Length (m)",
              "Expected Result", "Expected Error Code"]

    test_cases = [
        # (description, polygon_points_or_None, north, east, azimuth, length)
        ("Starts inside polygon and exits north", SQUARE_NED, 5, 5, 0, 100),
        ("Short ray entirely within polygon", SQUARE_NED, 5, 5, 0, 1),
        ("Ray goes north just outside west edge", SQUARE_NED, -5, -0.1, 0, 10),
        ("Ray lies exactly on west edge", SQUARE_NED, -5, 0, 0, 10),
        ("Azimuth 90 (East) crosses west edge", SQUARE_NED, 5, -5, 90, 20),
        ("Ray travels south over U-shape opening without touching solid", U_SHAPE_NED, 5, 15, 180, 4),
        ("Ray enters bay and hits inner wall", U_SHAPE_NED, 5, 5, 180, 5),
        ("Ray passes through U opening and stops before back wall", U_SHAPE_NED, 5, 12, 180, 6),
        # Error cases
        ("Null polygon pointer", None, 0, 0, 0, 10),
        ("Zero length ray is not allowed", SQUARE_NED, -5, 5, 0, 0),
        ("Polygon with fewer than 3 points", [(0, 0), (0, 10)], 0, 0, 0, 10),
    ]

    rows = []
    for desc, poly_pts, north, east, azimuth, length in test_cases:
        if poly_pts is None:
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointNE(north, east)
            lib.doesLineIntersectPolygonNED(None, 0, pt, ctypes.c_float(azimuth),
                                            ctypes.c_float(length),
                                            ctypes.byref(res), ctypes.byref(state))
            poly_str = "NULL"
        elif len(poly_pts) < 3:
            arr, count = make_ned_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointNE(north, east)
            lib.doesLineIntersectPolygonNED(arr, count, pt, ctypes.c_float(azimuth),
                                            ctypes.c_float(length),
                                            ctypes.byref(res), ctypes.byref(state))
            poly_str = format_ned_poly(poly_pts)
        else:
            arr, count = make_ned_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointNE(north, east)
            lib.doesLineIntersectPolygonNED(arr, count, pt, ctypes.c_float(azimuth),
                                            ctypes.c_float(length),
                                            ctypes.byref(res), ctypes.byref(state))
            poly_str = format_ned_poly(poly_pts)

        result_bool = "TRUE" if res.value == 1 else "FALSE"
        error_code = line_intersect_result_name(state.value)
        rows.append([desc, poly_str, north, east, azimuth, length, result_bool, error_code])

    filepath = os.path.join(qa_dir, "QA_doesLineIntersectPolygon.csv")
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
    print(f"  Written {len(rows)} test cases to {filepath}")


# =============================================================================
# QA_isInsidePolygonGeo.csv (NEW)
# =============================================================================
def generate_is_inside_polygon_geo(qa_dir):
    print("Generating QA_isInsidePolygonGeo.csv ...")
    header = ["Description", "Polygon Vertices (Lat/Lon/Alt in degrees/meters)",
              "Test Point Lat (deg)", "Test Point Lon (deg)", "Test Point Alt (m)",
              "Radius (m)", "Expected Result", "Expected Error Code"]

    test_cases = [
        # (description, polygon_points_or_None, lat, lon, alt, radius)
        ("Point at centre of geo square", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON, 0, 0),
        ("Point well outside geo square", SQUARE_GEO, 32.10, 34.79, 0, 0),
        ("Point near north edge (outside)", SQUARE_GEO, GEO_CENTER_LAT + GEO_HALF_LAT + 0.00045, GEO_CENTER_LON, 0, 0),
        ("Point far north (outside)", SQUARE_GEO, GEO_CENTER_LAT + GEO_HALF_LAT + 0.0027, GEO_CENTER_LON, 0, 0),
        ("Point near south edge (outside)", SQUARE_GEO, GEO_CENTER_LAT - GEO_HALF_LAT - 0.000027, GEO_CENTER_LON, 0, 0),
        ("Point on south edge", SQUARE_GEO, GEO_CENTER_LAT - GEO_HALF_LAT, GEO_CENTER_LON, 0, 0),
        ("Point on east edge", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON + GEO_HALF_LON, 0, 0),
        ("Point on closing (west) edge", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON - GEO_HALF_LON, 0, 0),
        ("Triangle inside", TRIANGLE_GEO, 32.084, 34.782, 0, 0),
        ("Triangle outside", TRIANGLE_GEO, 32.070, 34.770, 0, 0),
        ("Triangle on vertex", TRIANGLE_GEO, 32.080, 34.775, 0, 0),
        ("Point outside but radius 500m reaches edge", SQUARE_GEO, GEO_CENTER_LAT + GEO_HALF_LAT + 0.001, GEO_CENTER_LON, 0, 500),
        ("Point outside with small radius misses edge", SQUARE_GEO, GEO_CENTER_LAT + GEO_HALF_LAT + 0.001, GEO_CENTER_LON, 0, 10),
        # Error cases
        ("Null polygon pointer", None, 32.0, 34.0, 0, 0),
        ("Polygon with fewer than 3 points", [(32.0, 34.0, 0), (32.1, 34.1, 0)], 32.0, 34.0, 0, 0),
    ]

    rows = []
    for desc, poly_pts, lat, lon, alt, radius in test_cases:
        if poly_pts is None:
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointGeo(lat, lon, alt)
            lib.isInsidePolygonGeo(None, 0, pt, ctypes.c_float(radius),
                                   ctypes.byref(res), ctypes.byref(state))
            poly_str = "NULL"
        elif len(poly_pts) < 3:
            arr, count = make_geo_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointGeo(lat, lon, alt)
            lib.isInsidePolygonGeo(arr, count, pt, ctypes.c_float(radius),
                                   ctypes.byref(res), ctypes.byref(state))
            poly_str = format_geo_poly(poly_pts)
        else:
            arr, count = make_geo_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointGeo(lat, lon, alt)
            lib.isInsidePolygonGeo(arr, count, pt, ctypes.c_float(radius),
                                   ctypes.byref(res), ctypes.byref(state))
            poly_str = format_geo_poly(poly_pts)

        result_bool = "TRUE" if res.value == 1 else "FALSE"
        # Use same naming for geo errors
        geo_error_names = {0: "OK", 1: "POLYGON_IS_NULL_PTR", 2: "POLYGON_WITH_LESS_THAN_3_POINTS",
                           3: "OUTPUT_PTR_IS_NULL", 4: "POLYGON_EXCEEDS_MAX_VERTICES"}
        error_code = geo_error_names.get(state.value, f"UNKNOWN({state.value})")
        rows.append([desc, poly_str, lat, lon, alt, radius, result_bool, error_code])

    filepath = os.path.join(qa_dir, "QA_isInsidePolygonGeo.csv")
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
    print(f"  Written {len(rows)} test cases to {filepath}")


# =============================================================================
# QA_doesLineIntersectPolygonGeo.csv (NEW)
# =============================================================================
def generate_does_line_intersect_geo(qa_dir):
    print("Generating QA_doesLineIntersectPolygonGeo.csv ...")
    header = ["Description", "Polygon Vertices (Lat/Lon/Alt in degrees/meters)",
              "Start Point Lat (deg)", "Start Point Lon (deg)", "Start Point Alt (m)",
              "Azimuth (deg from North)", "Length (m)",
              "Expected Result", "Expected Error Code"]

    test_cases = [
        # (description, polygon_points_or_None, lat, lon, alt, azimuth, length)
        ("Line from inside going north", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON, 0, 0, 500),
        ("Line from outside crossing in east", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON - 0.01, 0, 90, 2000),
        ("Line going away from polygon", SQUARE_GEO, GEO_CENTER_LAT + 0.01, GEO_CENTER_LON, 0, 0, 500),
        ("Line too short to reach polygon", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON - 0.01, 0, 90, 100),
        ("Line crossing triangle east", TRIANGLE_GEO, 32.085, 34.770, 0, 90, 2000),
        ("Line missing triangle north", TRIANGLE_GEO, 32.095, 34.770, 0, 90, 2000),
        ("Short line from inside square", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON, 0, 45, 50),
        ("Line from south into square", SQUARE_GEO, GEO_CENTER_LAT - 0.005, GEO_CENTER_LON, 0, 0, 1000),
        # Error cases
        ("Null polygon pointer", None, 32.0, 34.0, 0, 0, 100),
        ("Zero length line", SQUARE_GEO, GEO_CENTER_LAT, GEO_CENTER_LON, 0, 0, 0),
        ("Polygon with fewer than 3 points", [(32.0, 34.0, 0), (32.1, 34.1, 0)], 32.0, 34.0, 0, 0, 100),
    ]

    rows = []
    for desc, poly_pts, lat, lon, alt, azimuth, length in test_cases:
        if poly_pts is None:
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointGeo(lat, lon, alt)
            lib.doesLineIntersectPolygonGeo(None, 0, pt, ctypes.c_float(azimuth),
                                            ctypes.c_float(length),
                                            ctypes.byref(res), ctypes.byref(state))
            poly_str = "NULL"
        elif len(poly_pts) < 3:
            arr, count = make_geo_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointGeo(lat, lon, alt)
            lib.doesLineIntersectPolygonGeo(arr, count, pt, ctypes.c_float(azimuth),
                                            ctypes.c_float(length),
                                            ctypes.byref(res), ctypes.byref(state))
            poly_str = format_geo_poly(poly_pts)
        else:
            arr, count = make_geo_poly(poly_pts)
            res = ctypes.c_uint8(0)
            state = ctypes.c_uint8(0)
            pt = SPointGeo(lat, lon, alt)
            lib.doesLineIntersectPolygonGeo(arr, count, pt, ctypes.c_float(azimuth),
                                            ctypes.c_float(length),
                                            ctypes.byref(res), ctypes.byref(state))
            poly_str = format_geo_poly(poly_pts)

        result_bool = "TRUE" if res.value == 1 else "FALSE"
        error_code = line_intersect_result_name(state.value)
        rows.append([desc, poly_str, lat, lon, alt, azimuth, length, result_bool, error_code])

    filepath = os.path.join(qa_dir, "QA_doesLineIntersectPolygonGeo.csv")
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
    print(f"  Written {len(rows)} test cases to {filepath}")


# =============================================================================
# QA_GeoToNed.csv
# =============================================================================
def generate_geo_to_ned(qa_dir):
    print("Generating QA_GeoToNed.csv ...")
    header = ["Description", "Origin Latitude (deg)", "Origin Longitude (deg)",
              "Origin Altitude (m)", "Input Latitude (deg)", "Input Longitude (deg)",
              "Input Altitude (m)", "Expected North (m)", "Expected East (m)",
              "Expected Down (m)", "Expected Error Code"]

    test_cases = [
        # (description, origin_lat, origin_lon, origin_alt, input_lat, input_lon, input_alt)
        ("Small displacement NE with altitude", 32.0, 34.0, 0.0, 32.01, 34.01, 50.0),
        ("Pure northward displacement", 32.0, 34.0, 0.0, 32.1, 34.0, 0.0),
        ("Pure eastward displacement", 32.0, 34.0, 0.0, 32.0, 34.1, 0.0),
        ("Negative north and east with altitude", 32.0, 34.0, 0.0, 31.9, 33.9, 100.0),
        ("Same point as origin produces zero NED", 32.0, 34.0, 0.0, 32.0, 34.0, 0.0),
        ("Only altitude differs", 32.0, 34.0, 0.0, 32.0, 34.0, 500.0),
    ]

    rows = []
    for desc, olat, olon, oalt, ilat, ilon, ialt in test_cases:
        origin = SPointGeo(olat, olon, oalt)
        input_pt = SPointGeo(ilat, ilon, ialt)
        result = SPointNED()
        lib.GeoToNed(origin, input_pt, ctypes.byref(result))
        rows.append([desc, olat, olon, oalt, ilat, ilon, ialt,
                     f"{result.north:.10f}", f"{result.east:.10f}",
                     f"{result.down:.10f}", "OK"])

    filepath = os.path.join(qa_dir, "QA_GeoToNed.csv")
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
    print(f"  Written {len(rows)} test cases to {filepath}")


# =============================================================================
# QA_NedToGeo.csv
# =============================================================================
def generate_ned_to_geo(qa_dir):
    print("Generating QA_NedToGeo.csv ...")
    header = ["Description", "Origin Latitude (deg)", "Origin Longitude (deg)",
              "Origin Altitude (m)", "Input North (m)", "Input East (m)",
              "Input Down (m)", "Expected Latitude (deg)", "Expected Longitude (deg)",
              "Expected Altitude (m)", "Expected Error Code"]

    test_cases = [
        # (description, origin_lat, origin_lon, origin_alt, north, east, down)
        ("Inverse of GeoToNed 1km NE case", 32.0, 34.0, 0.0, 1111.0, 943.0, -50.0),
        ("Pure northward NED to geographic", 32.0, 34.0, 0.0, 11132.0, 0.0, 0.0),
        ("Pure eastward NED to geographic", 32.0, 34.0, 0.0, 0.0, 9430.0, 0.0),
        ("Negative NED values to southwest", 32.0, 34.0, 0.0, -11132.0, -9430.0, -100.0),
        ("Zero NED returns origin coordinates", 32.0, 34.0, 0.0, 0.0, 0.0, 0.0),
        ("High altitude only", 32.0, 34.0, 0.0, 0.0, 0.0, -500.0),
    ]

    rows = []
    for desc, olat, olon, oalt, north, east, down in test_cases:
        origin = SPointGeo(olat, olon, oalt)
        ned_pt = SPointNED(north, east, down)
        result = SPointGeo()
        lib.NedToGeo(origin, ned_pt, ctypes.byref(result))
        rows.append([desc, olat, olon, oalt, north, east, down,
                     f"{result.latitudeDeg:.10f}", f"{result.longitudeDeg:.10f}",
                     f"{result.altitude:.10f}", "OK"])

    filepath = os.path.join(qa_dir, "QA_NedToGeo.csv")
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
    print(f"  Written {len(rows)} test cases to {filepath}")


# =============================================================================
# Main
# =============================================================================
def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    qa_dir = os.path.join(project_root, "qa")
    os.makedirs(qa_dir, exist_ok=True)

    print("=" * 60)
    print("  QA CSV GENERATION (from DLL)")
    print("=" * 60)

    generate_is_inside_polygon_ned(qa_dir)
    generate_does_line_intersect_ned(qa_dir)
    generate_is_inside_polygon_geo(qa_dir)
    generate_does_line_intersect_geo(qa_dir)
    generate_geo_to_ned(qa_dir)
    generate_ned_to_geo(qa_dir)

    print("\n" + "=" * 60)
    print("  DONE - All QA CSVs generated successfully")
    print("=" * 60)


if __name__ == "__main__":
    main()

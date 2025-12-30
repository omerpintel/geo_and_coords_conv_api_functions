import pymap3d as pm
import ctypes
import math
import random

import geo_utils


class SPointGeo(ctypes.Structure):
    _fields_ = [("latitudeDeg", ctypes.c_double),
                ("longitudeDeg", ctypes.c_double),
                ("altitude", ctypes.c_double)]

    def __repr__(self):
        return f"Geo(lat={self.latitudeDeg:.6f}, lon={self.longitudeDeg:.6f}, alt={self.altitude:.3f})"


class SPointNED(ctypes.Structure):
    # Adjust field names to match your C++ struct (e.g., x, y, z or north, east, down)
    _fields_ = [("north", ctypes.c_double),
                ("east", ctypes.c_double),
                ("down", ctypes.c_double)]

    def __repr__(self):
        return f"NED(N={self.north:.3f}, E={self.east:.3f}, D={self.down:.3f})"

lib = geo_utils.load_geopoint_library()

lib.GeoToNed.argtypes = [ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(SPointGeo)]
lib.GeoToNed.restype = SPointNED
# -----------------------
lib.NedToGeo.argtypes = [ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(ctypes.c_double),
                         ctypes.POINTER(SPointNED)]
lib.NedToGeo.restype = SPointGeo


def test_geo_to_ned():
    print("\n--- Testing GeoToNed ---")

    # Origin (Reference Point)
    lat0 = 32.0  # degrees
    lon0 = 34.0  # degrees
    alt0 = 0.0  # meters (pymap3d usually requires an origin altitude)

    # Target Point (Geodetic)
    # A point slightly North-East of origin
    target_geo = SPointGeo(32.01, 34.01, 50.0)

    # A. Get Expected Result from pymap3d
    # Note: pymap3d takes (lat, lon, alt, lat0, lon0, alt0)
    # We assume your NED origin is at altitude 0 relative to the ellipsoid or matches alt0
    expected_n, expected_e, expected_d = pm.geodetic2ned(
        target_geo.latitudeDeg, target_geo.longitudeDeg, target_geo.altitude,
        lat0, lon0, 0  # Assuming your API assumes origin is at 0 altitude or specific logic
    )

    ecef_x, ecef_y, ecef_z = pm.geodetic2ecef(target_geo.latitudeDeg, target_geo.longitudeDeg, target_geo.altitude)

    # B. Call C++ API
    c_lat0 = ctypes.c_double(lat0)
    c_lon0 = ctypes.c_double(lon0)

    result_ned = lib.GeoToNed(ctypes.byref(c_lat0),
                              ctypes.byref(c_lon0),
                              ctypes.byref(target_geo))

    # C. Compare
    print(f"Input Geo: {target_geo}")
    print(f"PyMap3D ecef : X={ecef_x:.6f}, Y={ecef_y:.6f}, Z={ecef_z:.6f}")
    print(f"PyMap3D : N={expected_n:.4f}, E={expected_e:.4f}, D={expected_d:.4f}")
    print(f"C++ API : N={result_ned.north:.4f}, E={result_ned.east:.4f}, D={result_ned.down:.4f}")

    # Allow small error (e.g. 1cm)
    assert math.isclose(result_ned.north, expected_n, abs_tol=0.01)
    assert math.isclose(result_ned.east, expected_e, abs_tol=0.01)
    print(">> SUCCESS: GeoToNed matches.")


def test_ned_to_geo():
    print("\n--- Testing NedToGeo ---")

    # Origin
    lat0 = 32.0
    lon0 = 34.0

    # Target Point (NED) -> 1000m North, 500m East, -50m Down (50m Up)
    target_ned = SPointNED(1000.0, 500.0, -50.0)

    # A. Get Expected Result from pymap3d
    exp_lat, exp_lon, exp_alt = pm.ned2geodetic(
        target_ned.north, target_ned.east, target_ned.down,
        lat0, lon0, 0
    )

    ecef_x,ecef_y,ecef_z = pm.ned2ecef(target_ned.north, target_ned.east, target_ned.down,lat0,lon0,0)

    # B. Call C++ API
    c_lat0 = ctypes.c_double(lat0)
    c_lon0 = ctypes.c_double(lon0)

    result_geo = lib.NedToGeo(ctypes.byref(c_lat0),
                              ctypes.byref(c_lon0),
                              ctypes.byref(target_ned))

    # C. Compare
    print(f"Input NED: {target_ned}")
    print(f"PyMap3D ecef : X={ecef_x:.6f}, Y={ecef_y:.6f}, Z={ecef_z:.6f}")
    print(f"PyMap3D : Lat={exp_lat:.6f}, Lon={exp_lon:.6f}, Alt={exp_alt:.3f}")
    print(
        f"C++ API : Lat={result_geo.latitudeDeg:.6f}, Lon={result_geo.longitudeDeg:.6f}, Alt={result_geo.altitude:.3f}")

    # Allow small error (approx 1e-6 degrees is ~11cm)
    assert math.isclose(result_geo.latitudeDeg, exp_lat, abs_tol=1e-6)
    assert math.isclose(result_geo.longitudeDeg, exp_lon, abs_tol=1e-6)
    print(">> SUCCESS: NedToGeo matches.")

def get_random_geo(center_lat, center_lon, radius_deg=1.0):
    """Generates a random Geo point within roughly radius_deg of center."""
    lat = center_lat + random.uniform(-radius_deg, radius_deg)
    lon = center_lon + random.uniform(-radius_deg, radius_deg)
    alt = random.uniform(-100, 1000)  # Random altitude
    return SPointGeo(lat, lon, alt)

def test_geo_to_ned_to_geo():
    print("\n--- Testing GeoToNedToGeo ---")

    # Origin (Reference Point)
    lat0 = 32.0  # degrees
    lon0 = 34.0  # degrees
    alt0 = 0.0  # meters (pymap3d usually requires an origin altitude)

    # Target Point (Geodetic)
    #target_geo = SPointGeo(32.01, 34.01, 50.0)
    target_geo = get_random_geo(lat0, lon0)


    # B. Call C++ API
    c_lat0 = ctypes.c_double(lat0)
    c_lon0 = ctypes.c_double(lon0)

    result_ned = lib.GeoToNed(ctypes.byref(c_lat0),
                              ctypes.byref(c_lon0),
                              ctypes.byref(target_geo))

    result_geo = lib.NedToGeo(ctypes.byref(c_lat0),
                              ctypes.byref(c_lon0),
                              ctypes.byref(result_ned))

    # C. Compare
    print(f"Input Geo: {target_geo}")
    print(f"C++ API : {result_geo}")

    # Allow small error (e.g. 1cm)
    assert math.isclose(result_geo.latitudeDeg, target_geo.latitudeDeg, abs_tol=1e-6)
    assert math.isclose(result_geo.longitudeDeg, target_geo.longitudeDeg, abs_tol=1e-6)
    assert math.isclose(result_geo.altitude, target_geo.altitude, abs_tol=0.01)
    print(">> SUCCESS: GeoToNed matches.")


if __name__ == "__main__":
    #test_ned_to_geo()
    #test_geo_to_ned()
    test_geo_to_ned_to_geo()
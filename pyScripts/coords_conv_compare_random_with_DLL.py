import ctypes
import math
import random
import pymap3d as pm
import pandas as pd
import plotly.express as px
import plotly.io as pio

# Enable browser rendering
pio.renderers.default = "browser"

import geo_utils

# --- Configuration ---
NUM_TESTS = 1000
LIB_PATH = "./libgeo_api.so"  # The loader handles the extension automatically


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
def get_random_geo(center_lat, center_lon, radius_deg=100.0):
    lat = center_lat + random.uniform(-radius_deg, radius_deg)
    lon = center_lon + random.uniform(-radius_deg, radius_deg)
    alt = random.uniform(-100, 1000)
    return SPointGeo(lat, lon, alt)


def get_random_ned(radius_m=500000.0):
    n = random.uniform(-radius_m, radius_m)
    e = random.uniform(-radius_m, radius_m)
    d = random.uniform(-500, 500)
    return SPointNED(n, e, d)


# --- 4. Test Logic ---
def run_stress_test():
    print(f"Starting Stress Test: {NUM_TESTS} random points...")

    # Fixed Origin
    lat0_val = 32.0
    lon0_val = 34.0
    c_lat0 = ctypes.c_double(lat0_val)
    c_lon0 = ctypes.c_double(lon0_val)

    results_data = []

    for i in range(NUM_TESTS):
        # -------------------------------------------------
        # TEST 1: Geo -> NED
        # -------------------------------------------------
        input_geo = get_random_geo(lat0_val, lon0_val)

        # Truth
        pm_n, pm_e, pm_d = pm.geodetic2ned(
            input_geo.latitudeDeg, input_geo.longitudeDeg, input_geo.altitude,
            lat0_val, lon0_val, 0
        )
        # Test
        res_ned = lib.GeoToNed(ctypes.byref(c_lat0), ctypes.byref(c_lon0), ctypes.byref(input_geo))

        # Error Calculation (Meters)
        diff_n = abs(res_ned.north - pm_n)
        diff_e = abs(res_ned.east - pm_e)
        diff_d = abs(res_ned.down - pm_d)
        total_err_m = math.sqrt(diff_n ** 2 + diff_e ** 2 + diff_d ** 2)

        results_data.append({
            "Test_Type": "GeoToNed",
            "Latitude": input_geo.latitudeDeg,
            "Longitude": input_geo.longitudeDeg,
            "Error_Distance_m": total_err_m
        })

        # -------------------------------------------------
        # TEST 2: NED -> Geo
        # -------------------------------------------------
        input_ned = get_random_ned()

        # Truth
        pm_lat, pm_lon, pm_alt = pm.ned2geodetic(
            input_ned.north, input_ned.east, input_ned.down,
            lat0_val, lon0_val, 0
        )
        # Test
        res_geo = lib.NedToGeo(ctypes.byref(c_lat0), ctypes.byref(c_lon0), ctypes.byref(input_ned))

        # Error Calculation (Degrees -> Meters Approximation)
        diff_lat_deg = abs(res_geo.latitudeDeg - pm_lat)
        diff_lon_deg = abs(res_geo.longitudeDeg - pm_lon)
        diff_alt_m = abs(res_geo.altitude - pm_alt)

        # Approx conversion factors
        meters_per_deg = 111132.0
        lat_err_m = diff_lat_deg * meters_per_deg
        lon_err_m = diff_lon_deg * meters_per_deg * math.cos(math.radians(lat0_val))

        total_geo_err_m = math.sqrt(lat_err_m ** 2 + lon_err_m ** 2 + diff_alt_m ** 2)

        results_data.append({
            "Test_Type": "NedToGeo",
            "Latitude": pm_lat,
            "Longitude": pm_lon,
            "Error_Distance_m": total_geo_err_m
        })

    # Convert to DataFrame
    df = pd.DataFrame(results_data)

    print("-" * 40)
    print(f"Completed {NUM_TESTS} cycles (Total {len(df)} data points).")

    # Changed to scientific notation to see tiny errors
    print(f"Max Error: {df['Error_Distance_m'].max():.2e} meters")

    # --- 5. Visualizations ---

    # Histogram
    print("Generating Histogram...")
    fig_hist = px.histogram(
        df,
        x="Error_Distance_m",
        color="Test_Type",
        nbins=50,
        barmode="overlay",
        title="Error Distribution: Geo->Ned vs Ned->Geo",
        labels={"Error_Distance_m": "Euclidean Distance Error (m)"},
        template="plotly_white",
        opacity=0.7
    )
    fig_hist.show()

    # Map (Fixed: Removed 'symbol')
    print("Generating Map...")
    fig_map = px.scatter_map(
        df,
        lat="Latitude",
        lon="Longitude",
        color="Error_Distance_m",
        # symbol="Test_Type",  <-- REMOVED THIS LINE
        size_max=15,
        zoom=8,
        title="Test Coverage & Error Magnitude",
        map_style="open-street-map",
        color_continuous_scale="Turbo",
        hover_data=["Error_Distance_m", "Test_Type"]  # Test_Type still visible on hover
    )
    fig_map.show()


if __name__ == "__main__":
    run_stress_test()
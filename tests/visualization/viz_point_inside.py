import ctypes
import os
import sys
import webbrowser
import random
import math
import folium

# Import shared logic
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'shared'))
import geo_utils
from geo_utils import SPointNE, SPointGeo, EResultState, EIsInsideGeoResult

def create_visualization(py_polygon, test_data, ORIGIN_LAT, ORIGIN_LON):
    print("\nGenerating Point-Inside Map (NED)...")
    m = folium.Map(location=[ORIGIN_LAT, ORIGIN_LON], zoom_start=16)

    # Draw Polygon
    poly_geo = [geo_utils.ned_to_geodetic(p.north, p.east, ORIGIN_LAT, ORIGIN_LON) for p in py_polygon]
    folium.Polygon(locations=poly_geo, color="#2E86C1", fill=True, fill_opacity=0.4).add_to(m)

    # Draw Points
    for pt, radius, inside in test_data:
        lat, lon = geo_utils.ned_to_geodetic(pt.north, pt.east, ORIGIN_LAT, ORIGIN_LON)
        color = "#EF4444" if inside else "#10B981" # Green/Red

        # Circle
        folium.Circle(
            location=[lat, lon], radius=radius, color=color, fill=True, fill_opacity=0.3,
            tooltip=f"Status: {'INSIDE' if inside else 'OUTSIDE'}<br>Rad: {radius:.1f}m"
        ).add_to(m)
        
        # Center Dot
        folium.CircleMarker(location=[lat, lon], radius=1, color="black").add_to(m)

    folder_name = "html_pages"
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)

    filename = "map_point_inside.html"
    output_path = os.path.join(folder_name, filename)
    m.save(output_path)
    webbrowser.open(f"file://{os.path.abspath(output_path)}")


def create_geo_visualization(geo_polygon_list, test_data, ORIGIN_LAT, ORIGIN_LON):
    print("\nGenerating Point-Inside Map (GEO)...")
    m = folium.Map(location=[ORIGIN_LAT, ORIGIN_LON], zoom_start=16)

    # Draw Polygon (already in lat/lon)
    poly_coords = [(p.latitudeDeg, p.longitudeDeg) for p in geo_polygon_list]
    folium.Polygon(locations=poly_coords, color="#8E44AD", fill=True, fill_opacity=0.4).add_to(m)

    # Draw Points
    for pt, radius, inside in test_data:
        color = "#EF4444" if inside else "#10B981"
        folium.Circle(
            location=[pt.latitudeDeg, pt.longitudeDeg], radius=radius, color=color, fill=True, fill_opacity=0.3,
            tooltip=f"Status: {'INSIDE' if inside else 'OUTSIDE'}<br>Rad: {radius:.1f}m"
        ).add_to(m)
        folium.CircleMarker(location=[pt.latitudeDeg, pt.longitudeDeg], radius=1, color="black").add_to(m)

    folder_name = "html_pages"
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)

    filename = "map_point_inside_geo.html"
    output_path = os.path.join(folder_name, filename)
    m.save(output_path)
    webbrowser.open(f"file://{os.path.abspath(output_path)}")


def main():
    lib = geo_utils.load_geopoint_library()
    
    # --- NED API ---
    lib.isInsidePolygonNED.argtypes = [
        ctypes.POINTER(SPointNE), ctypes.c_uint16, SPointNE, ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.isInsidePolygonNED.restype = None

    # --- GEO API ---
    lib.isInsidePolygonGeo.argtypes = [
        ctypes.POINTER(SPointGeo), ctypes.c_uint16, SPointGeo, ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.isInsidePolygonGeo.restype = None

    # --- Setup Data ---
    ORIGIN_LAT, ORIGIN_LON = 32.0853, 34.7818

    # ==============================
    # Part 1: NED Visualization
    # ==============================
    py_polygon = []
    for a in sorted([random.uniform(0, 2 * math.pi) for _ in range(12)]):
        r = random.uniform(300, 500)
        py_polygon.append(SPointNE(r * math.cos(a), r * math.sin(a)))
    
    c_polygon = (SPointNE * len(py_polygon))(*py_polygon)

    results = []
    for _ in range(50):
        r_pos, theta = 600 * math.sqrt(random.random()), random.uniform(0, 2*math.pi)
        pt = SPointNE(r_pos * math.cos(theta), r_pos * math.sin(theta))
        radius = random.uniform(5.0, 60.0)

        out_result = ctypes.c_uint8()
        out_state = ctypes.c_uint8()

        lib.isInsidePolygonNED(c_polygon, len(py_polygon), pt, radius, ctypes.byref(out_result), ctypes.byref(out_state))
        state_enum = EResultState(out_state.value)
        is_hit = bool(out_result.value)

        if state_enum == EResultState.IS_INSIDE_OK:
            results.append((pt, radius, is_hit))
        else:
            print(f"Error: {state_enum.name}")

    create_visualization(py_polygon, results, ORIGIN_LAT, ORIGIN_LON)

    # ==============================
    # Part 2: GEO Visualization
    # ==============================
    geo_polygon_list = []
    for a in sorted([random.uniform(0, 2 * math.pi) for _ in range(12)]):
        r = random.uniform(300, 500)
        # Convert random NED offset to geo
        lat, lon = geo_utils.ned_to_geodetic(r * math.cos(a), r * math.sin(a), ORIGIN_LAT, ORIGIN_LON)
        geo_polygon_list.append(SPointGeo(lat, lon, 0.0))

    c_geo_polygon = (SPointGeo * len(geo_polygon_list))(*geo_polygon_list)

    geo_results = []
    for _ in range(50):
        r_pos, theta = 600 * math.sqrt(random.random()), random.uniform(0, 2*math.pi)
        lat, lon = geo_utils.ned_to_geodetic(r_pos * math.cos(theta), r_pos * math.sin(theta), ORIGIN_LAT, ORIGIN_LON)
        pt = SPointGeo(lat, lon, 0.0)
        radius = random.uniform(5.0, 60.0)

        out_result = ctypes.c_uint8()
        out_state = ctypes.c_uint8()

        lib.isInsidePolygonGeo(c_geo_polygon, len(geo_polygon_list), pt, radius, ctypes.byref(out_result), ctypes.byref(out_state))
        state_enum = EIsInsideGeoResult(out_state.value)
        is_hit = bool(out_result.value)

        if state_enum == EIsInsideGeoResult.IS_INSIDE_GEO_OK:
            geo_results.append((pt, radius, is_hit))
        else:
            print(f"GEO Error: {state_enum.name}")

    create_geo_visualization(geo_polygon_list, geo_results, ORIGIN_LAT, ORIGIN_LON)

if __name__ == "__main__":
    main()
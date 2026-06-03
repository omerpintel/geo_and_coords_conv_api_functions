import ctypes
import os
import sys
import webbrowser
import random
import math
import folium

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'shared'))
import geo_utils
from geo_utils import SPointNE, SPointGeo, EResultState, ELineIntersectGeoResult

def create_visualization(py_polygon, line_results, ORIGIN_LAT, ORIGIN_LON):
    print("\nGenerating Line Intersection Map (NED)...")
    m = folium.Map(location=[ORIGIN_LAT, ORIGIN_LON], zoom_start=15)

    # Draw Polygon
    poly_geo = [geo_utils.ned_to_geodetic(p.north, p.east, ORIGIN_LAT, ORIGIN_LON) for p in py_polygon]
    folium.Polygon(locations=poly_geo, color="#2E86C1", fill=True, fill_opacity=0.4).add_to(m)

    # Draw Lines
    for start_pt, azimuth, length, intersects in line_results:
        theta = math.radians(azimuth)
        end_n = start_pt.north + length * math.cos(theta)
        end_e = start_pt.east + length * math.sin(theta)

        start_geo = geo_utils.ned_to_geodetic(start_pt.north, start_pt.east, ORIGIN_LAT, ORIGIN_LON)
        end_geo = geo_utils.ned_to_geodetic(end_n, end_e, ORIGIN_LAT, ORIGIN_LON)

        color = "#EF4444" if intersects else "#10B981" # Red if hit
        
        folium.PolyLine(
            locations=[start_geo, end_geo], color=color, weight=4, opacity=0.8,
            tooltip=f"Hit: {intersects}<br>Az: {azimuth:.1f}<br>Len: {length:.1f}"
        ).add_to(m)
        
        folium.CircleMarker(location=start_geo, radius=3, color="black", fill=True).add_to(m)

    folder_name = "html_pages"
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)

    filename = "map_line_intersect.html"
    output_path = os.path.join(folder_name, filename)
    m.save(output_path)
    webbrowser.open(f"file://{os.path.abspath(output_path)}")


def create_geo_visualization(geo_polygon_list, line_results, ORIGIN_LAT, ORIGIN_LON):
    print("\nGenerating Line Intersection Map (GEO)...")
    m = folium.Map(location=[ORIGIN_LAT, ORIGIN_LON], zoom_start=15)

    # Draw Polygon (already lat/lon)
    poly_coords = [(p.latitudeDeg, p.longitudeDeg) for p in geo_polygon_list]
    folium.Polygon(locations=poly_coords, color="#8E44AD", fill=True, fill_opacity=0.4).add_to(m)

    # Draw Lines
    EARTH_R = 6371000.0
    for start_pt, azimuth, length, intersects in line_results:
        # Compute endpoint using spherical formula (Python approximation for display)
        lat1 = math.radians(start_pt.latitudeDeg)
        lon1 = math.radians(start_pt.longitudeDeg)
        brng = math.radians(azimuth)
        d_r = length / EARTH_R
        lat2 = math.asin(math.sin(lat1)*math.cos(d_r) + math.cos(lat1)*math.sin(d_r)*math.cos(brng))
        lon2 = lon1 + math.atan2(math.sin(brng)*math.sin(d_r)*math.cos(lat1), math.cos(d_r) - math.sin(lat1)*math.sin(lat2))
        end_lat = math.degrees(lat2)
        end_lon = math.degrees(lon2)

        color = "#EF4444" if intersects else "#10B981"

        folium.PolyLine(
            locations=[(start_pt.latitudeDeg, start_pt.longitudeDeg), (end_lat, end_lon)],
            color=color, weight=4, opacity=0.8,
            tooltip=f"Hit: {intersects}<br>Az: {azimuth:.1f}<br>Len: {length:.1f}"
        ).add_to(m)

        folium.CircleMarker(location=[start_pt.latitudeDeg, start_pt.longitudeDeg], radius=3, color="black", fill=True).add_to(m)

    folder_name = "html_pages"
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)

    filename = "map_line_intersect_geo.html"
    output_path = os.path.join(folder_name, filename)
    m.save(output_path)
    webbrowser.open(f"file://{os.path.abspath(output_path)}")


def main():
    lib = geo_utils.load_geopoint_library()

    # --- NED API ---
    lib.doesLineIntersectPolygonNED.argtypes = [
        ctypes.POINTER(SPointNE), ctypes.c_uint16, SPointNE, ctypes.c_float, ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.doesLineIntersectPolygonNED.restype = None

    # --- GEO API ---
    lib.doesLineIntersectPolygonGeo.argtypes = [
        ctypes.POINTER(SPointGeo), ctypes.c_uint16, SPointGeo, ctypes.c_float, ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.doesLineIntersectPolygonGeo.restype = None

    ORIGIN_LAT, ORIGIN_LON = 32.0853, 34.7818

    # ==============================
    # Part 1: NED Visualization
    # ==============================
    py_polygon = []
    for a in sorted([random.uniform(0, 2 * math.pi) for _ in range(16)]):
        r = random.uniform(300, 700)
        py_polygon.append(SPointNE(r * math.cos(a), r * math.sin(a)))
    
    c_polygon = (SPointNE * len(py_polygon))(*py_polygon)

    line_results = []
    for _ in range(50):
        r_pos, theta = 1000 * math.sqrt(random.random()), random.uniform(0, 2*math.pi)
        start_pt = SPointNE(r_pos * math.cos(theta), r_pos * math.sin(theta))
        az, length = random.uniform(0, 360), random.uniform(100, 800)

        out_result = ctypes.c_uint8()
        out_state = ctypes.c_uint8()

        lib.doesLineIntersectPolygonNED(c_polygon, len(py_polygon), start_pt, az, length, out_result, out_state)
        state_enum = EResultState(out_state.value)
        is_hit = bool(out_result.value)

        if state_enum == EResultState.IS_INSIDE_OK:
            line_results.append((start_pt, az, length, is_hit))
        else:
            print(f"Error: {state_enum.name}")

    create_visualization(py_polygon, line_results, ORIGIN_LAT, ORIGIN_LON)

    # ==============================
    # Part 2: GEO Visualization
    # ==============================
    geo_polygon_list = []
    for a in sorted([random.uniform(0, 2 * math.pi) for _ in range(16)]):
        r = random.uniform(300, 700)
        lat, lon = geo_utils.ned_to_geodetic(r * math.cos(a), r * math.sin(a), ORIGIN_LAT, ORIGIN_LON)
        geo_polygon_list.append(SPointGeo(lat, lon, 0.0))

    c_geo_polygon = (SPointGeo * len(geo_polygon_list))(*geo_polygon_list)

    geo_line_results = []
    for _ in range(50):
        r_pos, theta = 1000 * math.sqrt(random.random()), random.uniform(0, 2*math.pi)
        lat, lon = geo_utils.ned_to_geodetic(r_pos * math.cos(theta), r_pos * math.sin(theta), ORIGIN_LAT, ORIGIN_LON)
        start_pt = SPointGeo(lat, lon, 0.0)
        az, length = random.uniform(0, 360), random.uniform(100, 800)

        out_result = ctypes.c_uint8()
        out_state = ctypes.c_uint8()

        lib.doesLineIntersectPolygonGeo(c_geo_polygon, len(geo_polygon_list), start_pt, az, length, out_result, out_state)
        state_enum = ELineIntersectGeoResult(out_state.value)
        is_hit = bool(out_result.value)

        if state_enum == ELineIntersectGeoResult.LINE_INTERSECT_GEO_OK:
            geo_line_results.append((start_pt, az, length, is_hit))
        else:
            print(f"GEO Error: {state_enum.name}")

    create_geo_visualization(geo_polygon_list, geo_line_results, ORIGIN_LAT, ORIGIN_LON)

if __name__ == "__main__":
    main()
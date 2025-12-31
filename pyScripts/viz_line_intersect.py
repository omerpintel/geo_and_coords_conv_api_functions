import ctypes
import os
import sys
import webbrowser
import random
import math
import folium

import geo_utils
from geo_utils import SPointNE, EResultState

def create_visualization(py_polygon, line_results, ORIGIN_LAT, ORIGIN_LON):
    print("\nGenerating Line Intersection Map...")
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

def main():
    lib = geo_utils.load_geopoint_library()

    lib.doesLineIntersectPolygon.argtypes = [
        ctypes.POINTER(SPointNE), ctypes.c_uint16, SPointNE, ctypes.c_float, ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.doesLineIntersectPolygon.restype = None

    ORIGIN_LAT, ORIGIN_LON = 32.0853, 34.7818
    origin_pt = SPointNE(0, 0)

    # Generate Polygon
    py_polygon = []
    for a in sorted([random.uniform(0, 2 * math.pi) for _ in range(16)]):
        r = random.uniform(300, 700)
        py_polygon.append(SPointNE(r * math.cos(a), r * math.sin(a)))
    
    c_polygon = (SPointNE * len(py_polygon))(*py_polygon)

    # Generate Lines
    line_results = []
    for _ in range(50):
        r_pos, theta = 1000 * math.sqrt(random.random()), random.uniform(0, 2*math.pi)
        start_pt = SPointNE(r_pos * math.cos(theta), r_pos * math.sin(theta))
        az, length = random.uniform(0, 360), random.uniform(100, 800)

        out_result = ctypes.c_uint8()
        out_state = ctypes.c_uint8()

        lib.doesLineIntersectPolygon(c_polygon, len(py_polygon), start_pt, az, length, out_result, out_state)
        state_enum = EResultState(out_state.value)
        is_hit = bool(out_result.value)

        if state_enum == EResultState.OK:
            # Process Valid Result
            line_results.append((start_pt, az, length, is_hit))
        else:
            print(f"Error: {state_enum.name}")

    create_visualization(py_polygon, line_results, ORIGIN_LAT, ORIGIN_LON)

if __name__ == "__main__":
    main()
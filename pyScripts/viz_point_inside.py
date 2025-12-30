import ctypes
import os
import sys
import webbrowser
import random
import math
import folium

# Import shared logic
import geo_utils
from geo_utils import Point, EResultState

def create_visualization(py_polygon, test_data, ORIGIN_LAT, ORIGIN_LON):
    print("\nGenerating Point-Inside Map...")
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

def main():
    lib = geo_utils.load_geopoint_library()
    
    # Define Signatures
    lib.isInsidePolygon.argtypes = [
        ctypes.POINTER(Point), ctypes.c_uint16, ctypes.POINTER(Point), ctypes.c_float, ctypes.POINTER(ctypes.c_bool), ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.isInsidePolygon.restype = None

    # --- Setup Data ---
    ORIGIN_LAT, ORIGIN_LON = 32.0853, 34.7818
    origin_pt = Point(0, 0)

    # Generate Polygon
    py_polygon = []
    for a in sorted([random.uniform(0, 2 * math.pi) for _ in range(12)]):
        r = random.uniform(300, 500)
        py_polygon.append(Point(r * math.cos(a), r * math.sin(a)))
    
    c_polygon = (Point * len(py_polygon))(*py_polygon)

    # Generate Test Data
    results = []
    for _ in range(50):
        r_pos, theta = 600 * math.sqrt(random.random()), random.uniform(0, 2*math.pi)
        pt = Point(r_pos * math.cos(theta), r_pos * math.sin(theta))
        radius = random.uniform(5.0, 60.0)

        out_result = ctypes.c_bool()
        out_state = ctypes.c_uint8()

        lib.isInsidePolygon(c_polygon, len(py_polygon), ctypes.byref(pt), radius, ctypes.byref(out_result), ctypes.byref(out_state))
        state_enum = EResultState(out_state.value)
        is_hit = out_result.value

        if state_enum == EResultState.OK:
            # Process Valid Result
            results.append((pt, radius, is_hit))
        else:
            print(f"Error: {state_enum.name}")

    create_visualization(py_polygon, results, ORIGIN_LAT, ORIGIN_LON)

if __name__ == "__main__":
    main()
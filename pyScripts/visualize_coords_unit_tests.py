import plotly.graph_objects as go
from plotly.subplots import make_subplots
import os
import webbrowser
import math

# --- Configuration ---
LOG_FILE_PATH = r"C:\Users\OMERPI\Desktop\GeoPointProject\out\build\x64-Debug\bin\test_results_wgs84.log"
# ---------------------

WGS84_A = 6378137.0


def parse_vec3(str_val):
    try:
        if not str_val or str_val == "NULL": return (0.0, 0.0, 0.0)
        return tuple(map(float, str_val.split(",")))
    except:
        return (0.0, 0.0, 0.0)


def plot_wgs84_tests(logfile):
    if not os.path.exists(logfile):
        print(f"Error: Log file not found at: {logfile}")
        return

    with open(logfile, "r") as f:
        lines = [l.strip() for l in f.readlines() if l.strip()]

    valid_data = []

    for i, line in enumerate(lines):
        parts = line.split("|")
        if len(parts) < 6: continue

        name = parts[0]
        test_type = parts[1]  # 'wgs84_conv' or 'wgs84_inv'

        vec_in = parse_vec3(parts[2])
        vec_exp = parse_vec3(parts[3])
        vec_act = parse_vec3(parts[4])
        passed_flag = (parts[5] == "1")

        # logic specific to test type
        error_m = 0.0
        plot_lat_deg = 0.0
        plot_lon_deg = 0.0

        if test_type == "wgs84_conv":
            # Input: Geo, Output: ECEF
            # Error is simple Euclidean distance in meters
            dx = vec_act[0] - vec_exp[0]
            dy = vec_act[1] - vec_exp[1]
            dz = vec_act[2] - vec_exp[2]
            error_m = math.sqrt(dx * dx + dy * dy + dz * dz)

            # For mapping
            plot_lat_deg = math.degrees(vec_in[0])
            plot_lon_deg = math.degrees(vec_in[1])

        elif test_type == "wgs84_inv":
            # Input: ECEF, Output: Geo
            # Error is in Radians/Meters. We approximate to Meters for the graph.
            dLat_rad = vec_act[0] - vec_exp[0]
            dLon_rad = vec_act[1] - vec_exp[1]
            dAlt_m = vec_act[2] - vec_exp[2]

            # Approx meters per radian (at equator roughly)
            lat_err_m = dLat_rad * WGS84_A
            lon_err_m = dLon_rad * WGS84_A * math.cos(vec_exp[0])

            error_m = math.sqrt(lat_err_m ** 2 + lon_err_m ** 2 + dAlt_m ** 2)

            # For mapping (Output is Geo, so use Expected Geo)
            plot_lat_deg = math.degrees(vec_exp[0])
            plot_lon_deg = math.degrees(vec_exp[1])

        valid_data.append({
            "name": name,
            "type": test_type,
            "geo_lat": plot_lat_deg,
            "geo_lon": plot_lon_deg,
            "passed": passed_flag,
            "error": error_m
        })

    if not valid_data:
        print("No valid data found.")
        return

    # --- Plotting ---
    fig = make_subplots(
        rows=3, cols=1,
        row_heights=[0.3, 0.3, 0.4],
        specs=[[{"type": "table"}], [{"type": "xy"}], [{"type": "geo"}]],
        subplot_titles=("Test Results Table", "Approx. Position Error (Meters)", "Test Coverage Map")
    )

    # 1. Table
    t_names = [d['name'] for d in valid_data]
    t_type = [d['type'] for d in valid_data]
    t_status = ["✅ PASS" if d['passed'] else "❌ FAIL" for d in valid_data]
    t_err = [f"{d['error']:.6f}" for d in valid_data]

    fig.add_trace(go.Table(
        header=dict(values=['Test Name', 'Type', 'Error (m)', 'Status'],
                    fill_color='paleturquoise', align='left'),
        cells=dict(values=[t_names, t_type, t_err, t_status],
                   fill_color='lavender', align='left')
    ), row=1, col=1)

    # 2. Bar Chart
    colors = ['#10B981' if d['passed'] else '#EF4444' for d in valid_data]
    fig.add_trace(go.Bar(x=t_names, y=[d['error'] for d in valid_data], marker_color=colors, name="Error (m)"), row=2,
                  col=1)

    # 3. Globe
    fig.add_trace(go.Scattergeo(
        lon=[d['geo_lon'] for d in valid_data],
        lat=[d['geo_lat'] for d in valid_data],
        mode='markers',
        marker=dict(size=8, color=colors),
        text=t_names, name="Locations"
    ), row=3, col=1)

    fig.update_layout(height=1000, title_text="WGS84 Round-Trip Test Report", showlegend=False)

    output_path = "wgs84_full_report.html"
    fig.write_html(output_path)
    print(f"Report generated: {output_path}")
    webbrowser.open('file://' + os.path.realpath(output_path))


if __name__ == "__main__":
    plot_wgs84_tests(LOG_FILE_PATH)
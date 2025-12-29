import plotly.graph_objects as go
from plotly.subplots import make_subplots
import os
import webbrowser
import math

# --- Configuration ---
LOG_FILE_PATH = r"C:\Users\OMERPI\Desktop\GeoPointProject\out\build\x64-Debug\bin\test_results_ned_geo.log"
# ---------------------

METERS_PER_DEG_LAT = 111319.9


def parse_vec3(str_val):
    try:
        if not str_val or str_val == "NULL": return (0.0, 0.0, 0.0)
        return tuple(map(float, str_val.split(",")))
    except:
        return (0.0, 0.0, 0.0)


def plot_ned_geo_tests(logfile):
    if not os.path.exists(logfile):
        print(f"Error: Log file not found at: {logfile}")
        return

    with open(logfile, "r") as f:
        lines = [l.strip() for l in f.readlines() if l.strip()]

    # Data Containers
    traces_pass = {'lat': [], 'lon': [], 'text': []}
    traces_fail = {'lat': [], 'lon': [], 'text': []}

    # For Mapbox lines, we need a list of independent line segments
    # Mapbox traces don't support "breaking" lines with None easily in all versions,
    # but creating a single trace with None separators usually works for visual gaps.
    line_lats = []
    line_lons = []

    table_data = []
    all_lats = []
    all_lons = []

    for line in lines:
        parts = line.split("|")
        if len(parts) < 6: continue

        name, test_type = parts[0], parts[1]
        vec_in = parse_vec3(parts[2])
        vec_exp = parse_vec3(parts[3])
        vec_act = parse_vec3(parts[4])
        passed = (parts[5] == "1")

        error_m = 0.0
        plot_lat = 0.0
        plot_lon = 0.0
        hover_detail = ""

        # --- Logic ---
        if test_type == "geo_to_ned":
            plot_lat, plot_lon = vec_in[0], vec_in[1]
            dn = vec_act[0] - vec_exp[0]
            de = vec_act[1] - vec_exp[1]
            dd = vec_act[2] - vec_exp[2]
            error_m = math.sqrt(dn ** 2 + de ** 2 + dd ** 2)

            hover_detail = (f"<b>{name}</b><br>Type: Geo->NED<br>Err: {error_m:.3f}m")

        elif test_type == "ned_to_geo":
            plot_lat, plot_lon = vec_exp[0], vec_exp[1]
            dLat_deg = vec_act[0] - vec_exp[0]
            dLon_deg = vec_act[1] - vec_exp[1]
            dAlt_m = vec_act[2] - vec_exp[2]

            lat_err_m = dLat_deg * METERS_PER_DEG_LAT
            lon_err_m = dLon_deg * METERS_PER_DEG_LAT * math.cos(math.radians(plot_lat))
            error_m = math.sqrt(lat_err_m ** 2 + lon_err_m ** 2 + dAlt_m ** 2)

            hover_detail = (f"<b>{name}</b><br>Type: NED->Geo<br>Err: {error_m:.3f}m<br>"
                            f"Act Lat: {vec_act[0]:.7f}<br>Act Lon: {vec_act[1]:.7f}")

            # Add error line for failed tests
            if not passed:
                line_lats.extend([vec_exp[0], vec_act[0], None])
                line_lons.extend([vec_exp[1], vec_act[1], None])

        # --- Storage ---
        all_lats.append(plot_lat)
        all_lons.append(plot_lon)
        table_data.append({'name': name, 'type': test_type, 'error': error_m, 'passed': passed})

        if passed:
            traces_pass['lat'].append(plot_lat)
            traces_pass['lon'].append(plot_lon)
            traces_pass['text'].append(hover_detail)
        else:
            traces_fail['lat'].append(plot_lat)
            traces_fail['lon'].append(plot_lon)
            traces_fail['text'].append(hover_detail)

    if not table_data:
        print("No valid data found.")
        return

    # --- Calculate Map Center ---
    center_lat = sum(all_lats) / len(all_lats) if all_lats else 0
    center_lon = sum(all_lons) / len(all_lons) if all_lons else 0

    # --- Plot Construction ---
    fig = make_subplots(
        rows=2, cols=2,
        column_widths=[0.6, 0.4],
        row_heights=[0.3, 0.7],
        specs=[[{"type": "xy"}, {"type": "table", "rowspan": 2}],
               [{"type": "mapbox", "colspan": 1}, None]],  # Changed "geo" to "mapbox"
        subplot_titles=("Error Magnitude", "Test Log", "Interactive Map (Scroll to Zoom)")
    )

    # 1. Bar Chart
    fig.add_trace(go.Bar(
        x=[d['name'] for d in table_data],
        y=[d['error'] for d in table_data],
        marker_color=['#10B981' if d['passed'] else '#EF4444' for d in table_data],
        name="Error (m)"
    ), row=1, col=1)

    # 2. Table
    fig.add_trace(go.Table(
        header=dict(values=['Test', 'Err (m)', 'Status'],
                    fill_color='#374151', font=dict(color='white'), align='left'),
        cells=dict(values=[
            [d['name'] for d in table_data],
            [f"{d['error']:.4f}" for d in table_data],
            ["✅" if d['passed'] else "❌" for d in table_data]
        ], fill_color='whitesmoke', align='left')
    ), row=1, col=2)

    # 3. Mapbox Traces
    # Passed
    fig.add_trace(go.Scattermapbox(
        lat=traces_pass['lat'], lon=traces_pass['lon'],
        mode='markers',
        marker=dict(size=12, color='#10B981'),
        text=traces_pass['text'], hoverinfo='text',
        name="Passed"
    ), row=2, col=1)

    # Failed
    fig.add_trace(go.Scattermapbox(
        lat=traces_fail['lat'], lon=traces_fail['lon'],
        mode='markers',
        marker=dict(size=14, color='#EF4444'),
        text=traces_fail['text'], hoverinfo='text',
        name="Failed"
    ), row=2, col=1)

    # Error Lines
    if line_lats:
        fig.add_trace(go.Scattermapbox(
            lat=line_lats, lon=line_lons,
            mode='lines',
            line=dict(width=2, color='red'),
            hoverinfo='skip',
            name="Error Offset"
        ), row=2, col=1)

    # --- Mapbox Layout ---
    fig.update_layout(
        height=900,
        title_text="Dynamic NED Debug Report",
        mapbox=dict(
            style="open-street-map",  # Full street detail, no key needed
            center=dict(lat=center_lat, lon=center_lon),
            zoom=2,  # Start zoomed out, user can zoom in infinitely
            domain={'x': [0, 0.6], 'y': [0, 0.65]}  # Manually position map in bottom-left
        ),
        legend=dict(orientation="h", y=1.02, x=0),
        margin=dict(l=10, r=10, t=50, b=10)
    )

    output_path = "dynamic_map_report.html"
    fig.write_html(output_path)
    print(f"Generated: {os.path.realpath(output_path)}")
    webbrowser.open('file://' + os.path.realpath(output_path))


if __name__ == "__main__":
    plot_ned_geo_tests(LOG_FILE_PATH)
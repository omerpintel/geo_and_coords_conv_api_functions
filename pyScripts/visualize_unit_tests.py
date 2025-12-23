import plotly.graph_objects as go
from plotly.subplots import make_subplots
import numpy as np
import os
import webbrowser


def plot_tests_interactive(logfile):
    if not os.path.exists(logfile):
        print(f"Error: {logfile} not found.")
        return

    with open(logfile, "r") as f:
        lines = [l.strip() for l in f.readlines() if l.strip()]

    valid_lines = []
    summary_data = []

    for i, line in enumerate(lines):
        parts = line.split("|")

        # We need at least 7 parts to draw (Name, Type, Poly, Point, V1, V2, Result)
        if len(parts) < 7:
            print(f"Skipping line {i + 1}: Not enough data fields.")
            continue

        name = parts[0]
        t_type = parts[1]
        poly_str = parts[2]
        p_str = parts[3]
        v1 = parts[4]
        v2 = parts[5]

        # --- FIXED LOGIC HERE ---
        # 8 Columns = Standard format (Name|Type|Poly|Point|V1|V2|Exp|Act)
        if len(parts) == 8:
            exp_val = parts[6]
            act_val = parts[7]
        # 7 Columns = Old format (Single result only)
        elif len(parts) == 7:
            exp_val = parts[6]
            act_val = parts[6]
        # 9 Columns = If you accidentally added extra separators
        elif len(parts) >= 9:
            exp_val = parts[7]  # Adjust if you have an extra column
            act_val = parts[8]
        else:
            exp_val = "0"
            act_val = "0"

        expected_bool = "True" if exp_val == "1" else "False"
        actual_bool = "True" if act_val == "1" else "False"
        match = (exp_val == act_val)

        # Store for plotting
        valid_lines.append({
            "name": name, "type": t_type, "poly": poly_str, "p": p_str,
            "v1": v1, "v2": v2, "exp": exp_val, "act": act_val
        })

        # Add status icon
        status_icon = "✅ PASS" if match else "❌ FAIL"
        summary_data.append([name, expected_bool, actual_bool, status_icon])

    if not valid_lines:
        print("No valid data found.")
        return

    # Create subplots
    titles = [f"{d['name']}<br>{'✅' if d['exp'] == d['act'] else '❌'} (Exp: {d['exp'] == '1'}, Act: {d['act'] == '1'})"
              for d in valid_lines]

    fig = make_subplots(
        rows=len(valid_lines) + 1,
        cols=1,
        subplot_titles=["Overall Test Summary"] + titles,
        vertical_spacing=0.02,
        specs=[[{"type": "table"}]] + [[{"type": "xy"}]] * len(valid_lines)
    )

    # Add Summary Table
    fig.add_trace(go.Table(
        header=dict(values=['Test Name', 'Expected', 'Actual', 'Status'], fill_color='paleturquoise', align='left'),
        cells=dict(values=list(zip(*summary_data)), fill_color='lavender', align='left')
    ), row=1, col=1)

    # Draw Each Plot
    for i, d in enumerate(valid_lines):
        row = i + 2

        # Draw Polygon
        if d['poly'] != "EMPTY":
            poly = [tuple(map(float, pt.split(","))) for pt in d['poly'].strip(";").split(";") if "," in pt]
            pn, pe = zip(*poly)
            # Close the loop for drawing
            fig.add_trace(go.Scatter(x=pe + (pe[0],), y=pn + (pn[0],), name="Polygon", fill="toself",
                                     line=dict(color='black', width=1), fillcolor='rgba(128, 128, 128, 0.1)'), row=row,
                          col=1)

        sn, se = map(float, d['p'].split(","))

        # Green if Actual is 1 (True), Red if 0 (False)
        color = '#EF4444' if d['act'] == "1" else '#10B981'

        if d['type'] == "circle":
            radius = float(d['v1'])
            theta = np.linspace(0, 2 * np.pi, 100)
            cx = se + radius * np.sin(theta)
            cy = sn + radius * np.cos(theta)
            fig.add_trace(go.Scatter(x=cx, y=cy, fill="toself", line=dict(color=color), fillcolor=color, opacity=0.3),
                          row=row, col=1)
            fig.add_trace(go.Scatter(x=[se], y=[sn], mode='markers', marker=dict(color='black', size=8)), row=row,
                          col=1)
        else:
            az, length = float(d['v1']), float(d['v2'])
            # NED to Cartesian
            en = sn + length * np.cos(np.radians(az))
            ee = se + length * np.sin(np.radians(az))
            fig.add_trace(go.Scatter(x=[se, ee], y=[sn, en], mode='lines+markers', line=dict(color=color, width=4)),
                          row=row, col=1)

        fig.update_xaxes(title_text="East (m)", row=row, col=1)
        fig.update_yaxes(title_text="North (m)", scaleanchor=f"x{row}", scaleratio=1, row=row, col=1)

    fig.update_layout(height=400 + (600 * len(valid_lines)), showlegend=False)
    fig.write_html("test_report.html")
    webbrowser.open('file://' + os.path.realpath("test_report.html"))


if __name__ == "__main__":
    # Update path if necessary
    path = r"C:\Users\OMERPI\Desktop\GeoPointProject\out\build\x64-Debug\bin\test_results.log"
    plot_tests_interactive(path)
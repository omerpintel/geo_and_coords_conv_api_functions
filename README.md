# GeoPoint — API Functions Library

A safety-critical C++ shared library for geometric analysis and geodetic coordinate transformations, operating under strict **stack-only memory** constraints.

## Core Capabilities

- **Point-in-Polygon (NED)**: Determines if a point/circle intersects a complex polygon in local NED coordinates (ray-casting algorithm)
- **Line-Polygon Intersection (NED)**: Detects if a directed line segment crosses polygon edges in NED coordinates
- **Point-in-Polygon (GEO)**: Spherical winding number algorithm for geodetic (lat/lon) polygons of any size
- **Line-Polygon Intersection (GEO)**: Great-circle arc intersection for geodetic coordinates
- **Coordinate Transformations**: ECEF ↔ Geodetic ↔ NED conversions using the WGS84 ellipsoid model

## Versioning

The library uses semantic versioning (`MAJOR.MINOR.PATCH`). The single source of truth is the
`project(ApiFunctions VERSION x.y.z ...)` line in the root `CMakeLists.txt`.

The configured version is exposed in three places:

- The public C API: `GetApiVersionString()` and `GetApiVersionNumbers(...)`
- The generated public header: `api_version.h`
- Shared library metadata: DLL file version on Windows and shared object version metadata on Linux

### Upgrading the Version

1. Edit only the version in the root `CMakeLists.txt`.
2. Reconfigure and rebuild the project so CMake regenerates `api_version.h` and binary metadata.
3. Run the sanity suite:

```bash
python tests/sanity/run_tests.py
```

4. Package the delivery artifacts:

```bash
python package.py
```

5. Confirm the validation report shows the expected API version and that DLL/shared-library export validation passes.

### Version Bump Rules

- Patch: bug fix or numeric accuracy improvement with no API/ABI change.
- Minor: new exported function or new capability without breaking existing consumers.
- Major: breaking API/ABI change, removed or renamed function, changed struct layout, or changed meaning of existing inputs/outputs.

## Algorithms

### `isInsidePolygonNED` — Ray Casting Algorithm (Even-Odd Rule)

**Algorithm:** [Ray Casting](https://en.wikipedia.org/wiki/Point_in_polygon#Ray_casting_algorithm) (also known as the Even-Odd Rule or Jordan Curve Theorem crossing number).

**How it works:**

1. A ray is cast from the test point towards the North direction (+X axis).
2. The algorithm counts how many polygon edges the ray crosses.
3. If the crossing count is **odd**, the point is inside; if **even**, the point is outside.
4. For each polygon edge, it checks if the edge straddles the test point's East coordinate, then computes the ray–edge intersection on the North axis using linear interpolation: $N_{intersect} = N_i + \frac{(N_j - N_i)}{(E_j - E_i)} \cdot (E_{test} - E_i)$
5. If the intersection point is strictly North of the test point, the crossing is toggled.

**Radius handling:** After the center-point test, if the center is outside, the algorithm checks if any polygon edge is closer than `radiusMeters` by computing the **squared perpendicular distance** from the point to each segment (avoiding expensive square root operations). If $d^2 < r^2$, the circle collides with the polygon boundary.

**Complexity:** $O(n)$ where $n$ is the number of polygon vertices.

---

### `doesLineIntersectPolygonNED` — Orientation-Based Segment Intersection

**Algorithm:** [Orientation Test](https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line_segment) using the cross-product orientation method for segment–segment intersection.

**How it works:**

1. **Start-point check:** First calls `isInsidePolygonNED` with radius=0 to test if the line's start point is already inside the polygon. If yes → immediate intersection.
2. **End-point computation:** Calculates the line segment's endpoint using azimuth and length:
   - $N_{end} = N_{start} + L \cdot \cos(\theta)$
   - $E_{end} = E_{start} + L \cdot \sin(\theta)$
   
   Where $\theta$ is the azimuth in radians (0=North, 90°=East) and $L$ is `maxLengthMeters`.
3. **Segment intersection test:** For each polygon edge $(P_i, P_{i+1})$, tests whether the line segment intersects using the **orientation triplet method**:
   - Computes the orientation (clockwise, counter-clockwise, or collinear) of four triplets formed by the two segment endpoint pairs.
   - **General case:** Two segments intersect if they "straddle" each other (orientations differ on both sides).
   - **Special cases:** Handles collinear points that lie on the other segment via bounding-box containment checks.

**Orientation function:** For three points $(P, Q, R)$, computes the cross product:
$$val = (Q_E - P_E)(R_N - Q_N) - (Q_N - P_N)(R_E - Q_E)$$
- $val > 0$ → Clockwise
- $val < 0$ → Counter-clockwise
- $val \approx 0$ → Collinear

**Complexity:** $O(n)$ where $n$ is the number of polygon vertices.

---

### `isInsidePolygonGeo` — Spherical Winding Number Algorithm

**Algorithm:** Winding number computed on the unit sphere via tangent-plane angle accumulation.

**How it works:**

1. All polygon vertices and the query point are converted from geodetic (lat/lon degrees) to **unit vectors** on the sphere: $\hat{v} = (\cos\phi\cos\lambda,\; \cos\phi\sin\lambda,\; \sin\phi)$
2. For each polygon edge $(V_i, V_{i+1})$, both vertices are projected onto the **tangent plane** at the query point by subtracting their component along $\hat{q}$: $T_i = V_i - (V_i \cdot \hat{q})\hat{q}$
3. The signed angle between consecutive projected vectors is accumulated: $\Delta\theta = \text{atan2}(\hat{q} \cdot (T_i \times T_{i+1}),\; T_i \cdot T_{i+1})$
4. If any edge's projected vectors are nearly anti-parallel (angle ≈ π), the point is on the boundary → **collision**.
5. After all edges (including the closing edge from last→first vertex), if $|\sum\Delta\theta| \approx 2\pi$, the point is **inside**.

**Radius handling:** If the center is outside, computes the **cross-track distance** from the query point to each polygon edge (great-circle arc, clamped to endpoints). If any distance < `radiusMeters`, the circle collides with the boundary.

**Cross-track distance formula:** For a point $P$ and a great-circle arc $AB$:
$$d_{xt} = R \cdot \left|\arcsin(\hat{n}_{AB} \cdot \hat{P})\right|$$
where $\hat{n}_{AB} = \text{normalize}(A \times B)$ is the pole of the great circle, with endpoint clamping via angular distance checks.

**Assumptions:** Simple polygon (non-self-intersecting), no antipodal vertices, altitude ignored (projected to sphere). Accurate at any scale (city-block to continent).

**Complexity:** $O(n)$ where $n$ is the number of polygon vertices.

---

### `doesLineIntersectPolygonGeo` — Great-Circle Arc Intersection

**Algorithm:** Computes intersection of great-circle arcs on the sphere.

**How it works:**

1. **Start-point check:** Calls `isInsidePolygonGeo` with radius=0. If inside → immediate intersection.
2. **End-point computation:** Uses the **destination point formula** to compute the line endpoint from start + azimuth + distance:
   - $\phi_2 = \arcsin(\sin\phi_1\cos\delta + \cos\phi_1\sin\delta\cos\theta)$
   - $\lambda_2 = \lambda_1 + \text{atan2}(\sin\theta\sin\delta\cos\phi_1,\; \cos\delta - \sin\phi_1\sin\phi_2)$
   
   Where $\delta = d/R$ is the angular distance and $\theta$ is the bearing.
3. **Arc-arc intersection test:** For each polygon edge, tests whether the two great-circle arcs intersect:
   - Computes great-circle plane normals: $\hat{n}_1 = \text{normalize}(A \times B)$, $\hat{n}_2 = \text{normalize}(C \times D)$
   - Candidate intersection points: $\hat{I} = \pm\text{normalize}(\hat{n}_1 \times \hat{n}_2)$
   - Verifies each candidate lies on **both** arcs using angular distance: $d(A,I) + d(I,B) \approx d(A,B)$

**Complexity:** $O(n)$ where $n$ is the number of polygon vertices.

---

### `GeoToNed` — Geodetic to Local Tangent Plane (NED) Conversion

**Algorithm:** Two-step conversion via ECEF (Earth-Centered, Earth-Fixed) using the **WGS84 ellipsoid model**.

**How it works:**

1. **Geodetic → ECEF** (`GeoToEcef`):
   - Computes the prime vertical radius of curvature: $R_N = \frac{a}{\sqrt{1 - e^2 \sin^2(\phi)}}$
   - Converts to Cartesian ECEF coordinates:
     - $X = (R_N + h) \cos\phi \cos\lambda$
     - $Y = (R_N + h) \cos\phi \sin\lambda$
     - $Z = (R_N(1 - e^2) + h) \sin\phi$
   
   Where $a = 6{,}378{,}137\,m$ (WGS84 semi-major axis), $e^2 = 2f - f^2$ (eccentricity squared), $\phi$ = latitude, $\lambda$ = longitude, $h$ = altitude.

2. **ECEF → NED** (`EcefToNed`):
   - Computes the ECEF difference vector: $\Delta\vec{r} = \vec{r}_{point} - \vec{r}_{origin}$
   - Applies the rotation matrix from ECEF to NED frame:

$$\begin{bmatrix} N \\ E \\ D \end{bmatrix} = \begin{bmatrix} -\sin\phi\cos\lambda & -\sin\phi\sin\lambda & \cos\phi \\ -\sin\lambda & \cos\lambda & 0 \\ -\cos\phi\cos\lambda & -\cos\phi\sin\lambda & -\sin\phi \end{bmatrix} \begin{bmatrix} \Delta X \\ \Delta Y \\ \Delta Z \end{bmatrix}$$

**WGS84 Parameters:**
| Constant | Value | Description |
|----------|-------|-------------|
| $a$ | 6,378,137 m | Semi-major axis |
| $f$ | 1/298.257223563 | Flattening |
| $e^2$ | 0.00669437999014 | Eccentricity squared |

---

### `NedToGeo` — Local Tangent Plane (NED) to Geodetic Conversion

**Algorithm:** Inverse of `GeoToNed` — converts NED back to geodetic via ECEF.

**How it works:**

1. **NED → ECEF** (`NedToEcef`):
   - Applies the **transpose** of the NED→ECEF rotation matrix (which is the inverse for orthogonal rotation matrices) to convert the NED vector back to an ECEF difference vector.
   - Adds the origin's ECEF position: $\vec{r}_{point} = R^T \cdot \vec{NED} + \vec{r}_{origin}$

2. **ECEF → Geodetic** (`EcefToGeo`):
   - Uses the **Bowring iterative method** (single iteration approximation) to recover latitude from ECEF:
     - Computes parametric latitude: $u = \arctan\left(\frac{z}{\sqrt{x^2 + y^2} \cdot \sqrt{1-e^2}}\right)$
     - Computes geodetic latitude: $\phi = \arctan\left(\frac{z + \frac{e^2}{1-e^2} \cdot a \cdot \sqrt{1-e^2} \cdot \sin^3 u}{\sqrt{x^2+y^2} - e^2 \cdot a \cdot \cos^3 u}\right)$
   - Longitude: $\lambda = \arctan2(y, x)$
   - Altitude computed differently depending on whether $\sin^2\phi \leq 0.5$ (near equator) or $> 0.5$ (near poles) for numerical stability.

**Complexity:** $O(1)$ — constant time per point conversion.

## Architectural Constraints

| Constraint | Enforcement |
|------------|-------------|
| No heap allocation | Global `operator new/delete` overridden → `std::abort()` in Debug, linker error in Release |
| Stack-only memory | Safety header force-included via CMake (`/FI` on MSVC, `-include` on GCC/Clang) |
| Max 128 polygon vertices | `MAX_POLYGON_VERTICES` enforced at runtime; exceeding returns error state |
| C ABI exported | All API functions use `extern "C"` for cross-language interop (Python ctypes, etc.) |
| Packed structs | `#pragma pack(push,1)` for deterministic memory layout |

## Building

### Prerequisites

- CMake 3.21+
- C++17 compiler (MSVC 2019+, GCC 9+, or Clang 10+)
- Ninja (recommended) or platform-default generator

### Using CMake Presets (Recommended)

The project includes `CMakePresets.json` for one-command configure/build/test.

**Windows (Developer PowerShell or VS Code):**
```powershell
cmake --preset windows-debug          # Configure
cmake --build --preset windows-debug   # Build
ctest --preset windows-debug           # Test
```

**Windows (Visual Studio generator):**
```powershell
cmake --preset windows-debug-vs
cmake --build --preset windows-debug-vs
ctest --preset windows-debug-vs
```

**Linux:**
```bash
cmake --preset linux-debug             # Configure
cmake --build --preset linux-debug      # Build
ctest --preset linux-debug              # Test
```

**Linux with AddressSanitizer:**
```bash
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
```

**Available Presets:**

| Preset | Platform | Config | Notes |
|--------|----------|--------|-------|
| `windows-debug` | Windows | Debug | Ninja + MSVC |
| `windows-release` | Windows | RelWithDebInfo | Ninja + MSVC |
| `windows-debug-vs` | Windows | Debug | Visual Studio 17 generator |
| `linux-debug` | Linux | Debug | Ninja + GCC |
| `linux-release` | Linux | RelWithDebInfo | Ninja + GCC |
| `linux-asan` | Linux | Debug + ASan | Memory error detection |

### Manual Configuration (Alternative)

**Windows:**
```powershell
cmake -S . -B out/build/x64-Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug
```

**Linux:**
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**With AddressSanitizer (Linux):**

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build
```

## Running Tests

### Quick (CTest)

```bash
cd build
ctest --output-on-failure
```

Or run individual test binaries:
```bash
./bin/geo_unit_tests        # Geometric logic + coverage verification
./bin/coords_conv_tests     # Coordinate conversions + robustness
```

### CI Test Suite (Full Validation with HTML Report)

The project includes a comprehensive test runner that builds, tests, validates DLL exports, and generates an HTML validation report.

**Windows:**
```powershell
# From project root
python run_tests.py                          # Debug build (default)
python run_tests.py --config Release         # Release build
python run_tests.py --build-dir .\build      # Custom build dir

# Or use the batch wrapper:
run_tests.bat
run_tests.bat Release
```

**Linux/macOS:**
```bash
# From project root
python3 run_tests.py                         # Debug build (default)
python3 run_tests.py --config Release        # Release build
python3 run_tests.py --asan                  # With AddressSanitizer

# Or use the shell wrapper:
chmod +x run_tests.sh
./run_tests.sh
./run_tests.sh Debug --asan
```

**CI Pipeline Integration:**
```yaml
# Example GitHub Actions step
- name: Run GeoPoint Validation Suite
  run: python run_tests.py --config Debug --asan
  
- name: Upload Test Report
  uses: actions/upload-artifact@v4
  with:
    name: test-report
    path: test_report.html
```

**What the CI suite validates:**

| Check | Description |
|-------|-------------|
| Build | Zero errors, zero warnings at `/W4` (MSVC) or `-Wall -Wextra` (GCC) |
| Geometric Tests | 83 tests: NED point-in-polygon, NED line intersection, GEO point-in-polygon, GEO line intersection, coverage verification (100% COV_POINT) |
| Conversion Tests | 110 tests: WGS84 round-trips, pole singularities, helper functions, robustness |
| DLL Exports | Verifies all 6 API functions are exported from the shared library |
| Memory Safety | No-heap enforcement validated (test runs without `std::abort()` triggering) |

**Output:** `test_report.html` — a self-contained HTML report with pass/fail badges, per-test results, build diagnostics, and environment info.

**Exit codes:** `0` = all passed, `1` = one or more failures.

## Deliveries

After a successful build, the final delivery artifacts are located under the **build output directory** (`<build_dir>/bin/` and `<build_dir>/lib/`).

| Artifact | Path | Description |
|----------|------|-------------|
| `api_functions.dll` | `<build_dir>/bin/` | Shared library (Windows) |
| `libapi_functions.so` | `<build_dir>/bin/` | Shared library (Linux) |
| `api_functions.lib` | `<build_dir>/lib/` | Import library for static linking (Windows) |
| `api_functions.h` | `include/` | API function declarations (DLL entry points) |
| `api_version.h` | `<build_dir>/generated/include/` | Generated API version constants |
| `api_structs.h` | `include/` | Struct and enum definitions (`SPointGeo`, `SPointNE`, `SPointNED`, enums) |
| `spherical_functions.h` | `include/` | Spherical geometry helper declarations (for advanced consumers) |

**Default build directories by preset:**

| Preset | Build Dir |
|--------|-----------|
| `windows-debug` | `out/build/windows-debug/` |
| `windows-release` | `out/build/windows-release/` |
| `windows-debug-vs` | `out/build/windows-debug-vs/` |
| `linux-debug` | `out/build/linux-debug/` |
| `linux-release` | `out/build/linux-release/` |
| CI runner (`run_tests.py`) | `out/build/ci/` |

**What to deliver to consumers:**
1. The shared library (`bin/api_functions.dll` or `bin/libapi_functions.so`)
2. The public headers (`include/api_functions.h` + `include/api_structs.h` + `include/api_version.h`)

The headers are sufficient for C/C++ integration. For Python/ctypes consumers, only the `.dll`/`.so` is needed (structs are redefined in Python).

### Packaging Script

Run `package.py` to collect all delivery artifacts into a `delivery/` folder at the project root:

```bash
python package.py                              # Auto-detect build dir
python package.py --preset windows-release     # From a specific preset
python package.py --build-dir out/build/ci     # From a specific build dir
```

This produces:
```
delivery/
├── api_functions.dll       # (or libapi_functions.so on Linux)
├── api_functions.lib       # (Windows only — import library)
└── include/
    ├── api_functions.h
    ├── api_version.h
    └── api_structs.h
```

## API Reference

### Version Functions

```c
const char* GetApiVersionString(void);

void GetApiVersionNumbers(
    uint16_t* major,   // [out, optional]
    uint16_t* minor,   // [out, optional]
    uint16_t* patch    // [out, optional]
);
```

`GetApiVersionString()` returns a static string in `MAJOR.MINOR.PATCH` format. Do not modify or free
the returned pointer. `GetApiVersionNumbers()` ignores null output pointers.

### Geometric Functions (NED)

```c
void isInsidePolygonNED(
    const SPointNE* polygon,    // Array of polygon vertices (NED meters)
    uint16_t pointCount,        // Number of vertices (3–128)
    const SPointNE testPoint,   // Test point center
    float radiusMeters,         // Circle radius (0 = point test)
    uint8_t* outResult,         // [out] 1 = collision, 0 = safe
    uint8_t* resultState        // [out] EIsInsideResult error code
);

void doesLineIntersectPolygonNED(
    const SPointNE* polygon,
    uint16_t pointCount,
    const SPointNE testPoint,   // Line start point
    float azimuthDegrees,       // Direction (0=North, 90=East)
    float maxLengthMeters,      // Line length
    uint8_t* outResult,
    uint8_t* resultState        // [out] ELineIntersectResult error code
);
```

### Geometric Functions (GEO / Spherical)

```c
void isInsidePolygonGeo(
    const SPointGeo* polygon,   // Array of polygon vertices (lat/lon degrees)
    uint16_t pointCount,        // Number of vertices (3–128)
    const SPointGeo testPoint,  // Test point (lat/lon degrees, altitude ignored)
    float radiusMeters,         // Circle radius on sphere surface (0 = point test)
    uint8_t* outResult,         // [out] 1 = collision, 0 = safe
    uint8_t* resultState        // [out] EIsInsideGeoResult error code
);

void doesLineIntersectPolygonGeo(
    const SPointGeo* polygon,
    uint16_t pointCount,
    const SPointGeo testPoint,  // Line start (lat/lon degrees)
    float azimuthDegrees,       // Initial bearing (0=North, 90=East)
    float maxLengthMeters,      // Line length along great circle
    uint8_t* outResult,         // [out] 1 = collision, 0 = safe
    uint8_t* resultState        // [out] ELineIntersectGeoResult error code
);
```

### Coordinate Transformations

```c
void GeoToNed(
    double originLatDeg, double originLonDeg, double originAlt,
    const SPointGeo geoPoint,
    SPointNED* resNedPoint
);

void NedToGeo(
    double originLatDeg, double originLonDeg, double originAlt,
    const SPointNED nedPoint,
    SPointGeo* resGeoPoint
);
```

### Error States (NED Functions)

| Value | Enum | Meaning |
|-------|------|---------|
| 0 | `IS_INSIDE_OK` / `LINE_INTERSECT_OK` | Operation succeeded |
| 1 | `POLYGON_WITH_LESS_THAN_3_POINTS` | Polygon must have ≥ 3 vertices |
| 2 | `POLYGON_IS_NULL_PTR` | Polygon pointer is null |
| 3 | `MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO` | Line length must be positive |
| 4 | `EXCEEDS_MAX_VERTICES` | Polygon exceeds 128 vertices |

### Error States (GEO Functions)

| Value | Enum | Meaning |
|-------|------|---------|
| 0 | `IS_INSIDE_GEO_OK` / `LINE_INTERSECT_GEO_OK` | Operation succeeded |
| 1 | `*_POLYGON_WITH_LESS_THAN_3_POINTS` | Polygon must have ≥ 3 vertices |
| 2 | `*_POLYGON_IS_NULL_PTR` | Polygon pointer is null |
| 3 | `*_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO` | Line length must be positive |
| 4 | `*_POLYGON_EXCEEDS_MAX_VERTICES` | Polygon exceeds 128 vertices |

## Python Visualization

Interactive visualization tools using `folium` and `plotly`:

```bash
cd pyScripts
python viz_point_inside.py           # Point-in-polygon map
python viz_line_intersect.py         # Line intersection map
python visualize_geo_unit_tests.py   # Test results dashboard
python coords_conv_compare_random_with_DLL.py  # Monte Carlo validation
```

### Python Dependencies

```
pip install folium plotly pymap3d pandas
```

## Project Structure

```
├── CMakeLists.txt              # Root build config + safety enforcement
├── include/
│   ├── api_functions.h         # DLL export API
│   ├── api_structs.h           # Data structures
│   ├── coords_conv_functions.h # WGS84 constants + declarations
│   ├── geometric_functions.h   # Geometry helper declarations
│   ├── spherical_functions.h   # Spherical geometry (GEO) declarations
│   ├── no_heap.h               # Heap allocation trap (declarations)
│   ├── cov_spy.h               # Coverage instrumentation
│   └── test_utils.h            # Coverage access (debug)
├── src/
│   ├── functions.cpp           # API implementations
│   ├── geometric_functions.cpp # Geometric algorithms (NED)
│   ├── spherical_functions.cpp # Spherical geometry algorithms (GEO)
│   ├── coords_conv_functions.cpp # Coordinate conversions
│   └── no_heap.cpp             # operator new/delete → abort()
├── tests/
│   ├── geometric_test.cpp      # Geometry tests + coverage
│   └── coords_conv_test.cpp    # Conversion + robustness tests
└── pyScripts/
    ├── geo_utils.py            # Shared DLL loader
    ├── viz_point_inside.py     # Folium point visualization
    ├── viz_line_intersect.py   # Folium line visualization
    ├── visualize_geo_unit_tests.py  # Plotly test dashboard
    ├── coords_conv_compare_with_DLL.py      # Single-point comparison
    └── coords_conv_compare_random_with_DLL.py # Monte Carlo stress test
```

## Memory Safety Verification

The no-heap policy is enforced at two levels:

1. **Compile-time (Debug)**: `no_heap.h` declares `operator new/delete` without definitions via forced include → any `new` expression produces a link error or runtime abort
2. **Link-time (Release)**: References to an undefined symbol `LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN` cause link failure if any allocation is attempted

To verify: build in Debug and confirm no dynamic allocation warnings. Build with ASan (`-DENABLE_ASAN=ON`) for runtime verification.

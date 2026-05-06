# GeoPoint — API Functions Library

A safety-critical C++ shared library for geometric analysis and geodetic coordinate transformations, operating under strict **stack-only memory** constraints.

## Core Capabilities

- **Point-in-Polygon**: Determines if a point/circle intersects a complex polygon (ray-casting algorithm)
- **Line-Polygon Intersection**: Detects if a directed line segment crosses polygon edges
- **Coordinate Transformations**: ECEF ↔ Geodetic ↔ NED conversions using the WGS84 ellipsoid model

## Architectural Constraints

| Constraint | Enforcement |
|------------|-------------|
| No heap allocation | Global `operator new/delete` overridden → `std::abort()` in Debug, linker error in Release |
| Stack-only memory | Safety header force-included via CMake (`/FI` on MSVC, `-include` on GCC/Clang) |
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
| Geometric Tests | 27 tests: point-in-polygon, line intersection, coverage verification (100% COV_POINT) |
| Conversion Tests | 110 tests: WGS84 round-trips, pole singularities, helper functions, robustness |
| DLL Exports | Verifies all 4 API functions are exported from the shared library |
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
| `api_structs.h` | `include/` | Struct and enum definitions (`SPointGeo`, `SPointNE`, `SPointNED`, enums) |

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
2. The two header files (`include/api_functions.h` + `include/api_structs.h`)

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
    └── api_structs.h
```

## API Reference

### Geometric Functions

```c
void isInsidePolygon(
    const SPointNE* polygon,    // Array of polygon vertices (NED meters)
    uint16_t pointCount,        // Number of vertices (>= 3)
    const SPointNE testPoint,   // Test point center
    float radiusMeters,         // Circle radius (0 = point test)
    uint8_t* outResult,         // [out] 1 = collision, 0 = safe
    uint8_t* resultState        // [out] EResultState error code
);

void doesLineIntersectPolygon(
    const SPointNE* polygon,
    uint16_t pointCount,
    const SPointNE testPoint,   // Line start point
    float azimuthDegrees,       // Direction (0=North, 90=East)
    float maxLengthMeters,      // Line length
    uint8_t* outResult,
    uint8_t* resultState
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

### Error States

| Value | Enum | Meaning |
|-------|------|---------|
| 0 | `OK` | Operation succeeded |
| 1 | `POLYGON_WITH_LESS_THAN_3_POINTS` | Polygon must have >= 3 vertices |
| 2 | `POLYGON_IS_NULL_PTR` | Polygon pointer is null |
| 3 | `MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO` | Line length must be positive |

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
│   ├── no_heap.h               # Heap allocation trap (declarations)
│   ├── cov_spy.h               # Coverage instrumentation
│   └── test_utils.h            # Coverage access (debug)
├── src/
│   ├── functions.cpp           # API implementations
│   ├── geometric_functions.cpp # Geometric algorithms
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

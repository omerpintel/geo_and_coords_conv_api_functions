#include "api_functions.h"
#include "coords_conv_functions.h"
#include "geometric_functions.h"

#include <iostream>
#include <cmath>
#include <cstdint>
#include <cfloat>
#include <cstring>

// --- Mini Test Framework ---
int g_tests_passed = 0;
int g_tests_failed = 0;

#define ASSERT_NEAR(actual, expected, tolerance, msg) \
    do { \
        double _a = (actual); \
        double _e = (expected); \
        double _t = (tolerance); \
        if (std::fabs(_a - _e) <= _t) { \
            std::cout << "[PASS] " << msg << std::endl; \
            g_tests_passed++; \
        } else { \
            std::cout << "[FAIL] " << msg << " | Expected: " << _e << ", Got: " << _a << ", Diff: " << std::fabs(_a - _e) << std::endl; \
            g_tests_failed++; \
        } \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "[PASS] " << msg << std::endl; \
            g_tests_passed++; \
        } else { \
            std::cout << "[FAIL] " << msg << std::endl; \
            g_tests_failed++; \
        } \
    } while(0)

#define ASSERT_FALSE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cout << "[PASS] " << msg << std::endl; \
            g_tests_passed++; \
        } else { \
            std::cout << "[FAIL] " << msg << std::endl; \
            g_tests_failed++; \
        } \
    } while(0)

// ========================================================================
// SECTION 0: API Version Tests
// ========================================================================

void test_api_version() {
    std::cout << "\n--- API Version ---\n";

    ASSERT_TRUE(std::strcmp(GetApiVersionString(), "0.1.0") == 0, "Version string is 0.1.0");

    uint16_t major = 99;
    uint16_t minor = 99;
    uint16_t patch = 99;
    GetApiVersionNumbers(&major, &minor, &patch);

    ASSERT_TRUE(major == 0, "Version major is 0");
    ASSERT_TRUE(minor == 1, "Version minor is 1");
    ASSERT_TRUE(patch == 0, "Version patch is 0");

    GetApiVersionNumbers(nullptr, nullptr, nullptr);
    std::cout << "[PASS] GetApiVersionNumbers with null outputs (no crash)" << std::endl;
    g_tests_passed++;
}

// ========================================================================
// SECTION 1: Coordinate Conversion Tests
// ========================================================================

void test_GeoToEcef_known_values() {
    std::cout << "\n--- GeoToEcef Known Values ---\n";

    // Equator, Prime Meridian, sea level
    {
        SPointGeo geo = {0.0, 0.0, 0.0};
        SPointECEF ecef = GeoToEcef(geo);
        ASSERT_NEAR(ecef.x, 6378137.0, 0.001, "Equator/PM: X = semi-major axis");
        ASSERT_NEAR(ecef.y, 0.0, 0.001, "Equator/PM: Y = 0");
        ASSERT_NEAR(ecef.z, 0.0, 0.001, "Equator/PM: Z = 0");
    }

    // Equator, 90E
    {
        SPointGeo geo = {0.0, 90.0, 0.0};
        SPointECEF ecef = GeoToEcef(geo);
        ASSERT_NEAR(ecef.x, 0.0, 0.001, "Equator/90E: X = 0");
        ASSERT_NEAR(ecef.y, 6378137.0, 0.001, "Equator/90E: Y = semi-major axis");
        ASSERT_NEAR(ecef.z, 0.0, 0.001, "Equator/90E: Z = 0");
    }

    // North Pole
    {
        SPointGeo geo = {90.0, 0.0, 0.0};
        SPointECEF ecef = GeoToEcef(geo);
        // At north pole: z = b (semi-minor axis) = a*(1-f) = 6356752.314
        double b = WGS84::A * (1.0 - WGS84::F);
        ASSERT_NEAR(ecef.x, 0.0, 0.001, "North Pole: X = 0");
        ASSERT_NEAR(ecef.y, 0.0, 0.001, "North Pole: Y = 0");
        ASSERT_NEAR(ecef.z, b, 0.01, "North Pole: Z = semi-minor axis");
    }

    // South Pole
    {
        SPointGeo geo = {-90.0, 0.0, 0.0};
        SPointECEF ecef = GeoToEcef(geo);
        double b = WGS84::A * (1.0 - WGS84::F);
        ASSERT_NEAR(ecef.x, 0.0, 0.001, "South Pole: X = 0");
        ASSERT_NEAR(ecef.y, 0.0, 0.001, "South Pole: Y = 0");
        ASSERT_NEAR(ecef.z, -b, 0.01, "South Pole: Z = -semi-minor axis");
    }

    // Date Line (180E)
    {
        SPointGeo geo = {0.0, 180.0, 0.0};
        SPointECEF ecef = GeoToEcef(geo);
        ASSERT_NEAR(ecef.x, -6378137.0, 0.001, "Date Line: X = -semi-major axis");
        ASSERT_NEAR(ecef.y, 0.0, 0.001, "Date Line: Y ~ 0");
        ASSERT_NEAR(ecef.z, 0.0, 0.001, "Date Line: Z = 0");
    }

    // High altitude
    {
        SPointGeo geo = {0.0, 0.0, 100000.0}; // 100km altitude
        SPointECEF ecef = GeoToEcef(geo);
        ASSERT_NEAR(ecef.x, 6378137.0 + 100000.0, 0.001, "100km alt: X = a + 100km");
    }
}

void test_EcefToGeo_known_values() {
    std::cout << "\n--- EcefToGeo Known Values ---\n";

    // Equator, Prime Meridian
    {
        SPointECEF ecef = {6378137.0, 0.0, 0.0};
        SPointGeo geo = EcefToGeo(ecef);
        ASSERT_NEAR(geo.latitudeDeg, 0.0, 1e-6, "Equator/PM: lat = 0");
        ASSERT_NEAR(geo.longitudeDeg, 0.0, 1e-6, "Equator/PM: lon = 0");
        ASSERT_NEAR(geo.altitude, 0.0, 0.01, "Equator/PM: alt = 0");
    }

    // North Pole
    {
        double b = WGS84::A * (1.0 - WGS84::F);
        SPointECEF ecef = {0.0, 0.0, b};
        SPointGeo geo = EcefToGeo(ecef);
        ASSERT_NEAR(geo.latitudeDeg, 90.0, 0.001, "North Pole: lat = 90");
        ASSERT_NEAR(geo.altitude, 0.0, 1.0, "North Pole: alt ~ 0");
    }

    // South Pole
    {
        double b = WGS84::A * (1.0 - WGS84::F);
        SPointECEF ecef = {0.0, 0.0, -b};
        SPointGeo geo = EcefToGeo(ecef);
        ASSERT_NEAR(geo.latitudeDeg, -90.0, 0.001, "South Pole: lat = -90");
        ASSERT_NEAR(geo.altitude, 0.0, 1.0, "South Pole: alt ~ 0");
    }

    // 90E Equator
    {
        SPointECEF ecef = {0.0, 6378137.0, 0.0};
        SPointGeo geo = EcefToGeo(ecef);
        ASSERT_NEAR(geo.latitudeDeg, 0.0, 1e-6, "90E Equator: lat = 0");
        ASSERT_NEAR(geo.longitudeDeg, 90.0, 1e-6, "90E Equator: lon = 90");
    }
}

void test_GeoToEcef_roundtrip() {
    std::cout << "\n--- GeoToEcef/EcefToGeo Round-Trip ---\n";

    // Test multiple points around the globe
    struct TestCase { double lat; double lon; double alt; const char* name; };
    TestCase cases[] = {
        {32.0, 34.0, 100.0, "Tel Aviv"},
        {51.5, -0.1, 50.0, "London"},
        {-33.9, 18.4, 0.0, "Cape Town"},
        {35.7, 139.7, 40.0, "Tokyo"},
        {-22.9, -43.2, 10.0, "Rio de Janeiro"},
        {0.0, 0.0, 0.0, "Null Island"},
        {89.99, 45.0, 0.0, "Near North Pole"},
        {-89.99, -120.0, 0.0, "Near South Pole"},
        {0.0, 179.99, 0.0, "Near Date Line"},
        {45.0, 90.0, 35000.0, "High Alt (35km)"},
    };

    for (const auto& tc : cases) {
        SPointGeo original = {tc.lat, tc.lon, tc.alt};
        SPointECEF ecef = GeoToEcef(original);
        SPointGeo recovered = EcefToGeo(ecef);

        double latErr = std::fabs(recovered.latitudeDeg - original.latitudeDeg);
        double lonErr = std::fabs(recovered.longitudeDeg - original.longitudeDeg);
        double altErr = std::fabs(recovered.altitude - original.altitude);

        // 1e-9 degrees ~ 0.1mm, altitude tolerance 1mm
        bool pass = (latErr < 1e-8) && (lonErr < 1e-8) && (altErr < 0.001);
        if (pass) {
            std::cout << "[PASS] Round-trip: " << tc.name << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Round-trip: " << tc.name
                      << " | lat_err=" << latErr << " lon_err=" << lonErr << " alt_err=" << altErr << std::endl;
            g_tests_failed++;
        }
    }
}

void test_NedRoundTrip() {
    std::cout << "\n--- NED Round-Trip (GeoToNed -> NedToGeo) ---\n";

    double originLat = 32.0;
    double originLon = 34.0;
    double originAlt = 0.0;

    struct TestCase { double lat; double lon; double alt; const char* name; };
    TestCase cases[] = {
        {32.01, 34.01, 50.0, "1km NE offset"},
        {32.1, 34.0, 0.0, "11km North"},
        {32.0, 34.1, 0.0, "9km East"},
        {31.9, 33.9, 100.0, "11km SW + alt"},
        {32.0, 34.0, 500.0, "Same position, high alt"},
        {32.0, 34.0, 0.0, "Origin itself"},
    };

    for (const auto& tc : cases) {
        SPointGeo inputGeo = {tc.lat, tc.lon, tc.alt};
        SPointGeo origin = {originLat, originLon, originAlt};
        SPointNED ned;
        GeoToNed(origin, inputGeo, &ned);

        SPointGeo recovered;
        NedToGeo(origin, ned, &recovered);

        double latErr = std::fabs(recovered.latitudeDeg - tc.lat);
        double lonErr = std::fabs(recovered.longitudeDeg - tc.lon);
        double altErr = std::fabs(recovered.altitude - tc.alt);

        // Tolerance: 1e-7 degrees ~ 1cm, altitude 1cm
        bool pass = (latErr < 1e-7) && (lonErr < 1e-7) && (altErr < 0.01);
        if (pass) {
            std::cout << "[PASS] NED Round-trip: " << tc.name << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] NED Round-trip: " << tc.name
                      << " | lat_err=" << latErr << " lon_err=" << lonErr << " alt_err=" << altErr << std::endl;
            g_tests_failed++;
        }
    }
}

void test_EcefToNed_NedToEcef_roundtrip() {
    std::cout << "\n--- EcefToNed/NedToEcef Round-Trip ---\n";

    double originLat = 45.0;
    double originLon = 10.0;
    double originAlt = 200.0;

    // Create a point, convert to NED, convert back
    SPointGeo testGeo = {45.05, 10.05, 300.0};
    SPointECEF ecef = GeoToEcef(testGeo);

    SPointNED ned = EcefToNed(originLat, originLon, originAlt, ecef);
    SPointECEF recovered = NedToEcef(originLat, originLon, originAlt, ned);

    ASSERT_NEAR(recovered.x, ecef.x, 0.001, "ECEF->NED->ECEF: X match");
    ASSERT_NEAR(recovered.y, ecef.y, 0.001, "ECEF->NED->ECEF: Y match");
    ASSERT_NEAR(recovered.z, ecef.z, 0.001, "ECEF->NED->ECEF: Z match");
}

void test_NavValidateLatitude() {
    std::cout << "\n--- NavValidateLatitude ---\n";

    // Normal range - should pass through
    ASSERT_NEAR(NavValidateLatitude(0.0), 0.0, 1e-15, "Validate lat: 0");
    ASSERT_NEAR(NavValidateLatitude(PI / 4.0), PI / 4.0, 1e-15, "Validate lat: PI/4");
    ASSERT_NEAR(NavValidateLatitude(-PI / 4.0), -PI / 4.0, 1e-15, "Validate lat: -PI/4");

    // At boundaries
    ASSERT_NEAR(NavValidateLatitude(PI / 2.0), PI / 2.0, 1e-15, "Validate lat: PI/2 (exact)");
    ASSERT_NEAR(NavValidateLatitude(-PI / 2.0), -PI / 2.0, 1e-15, "Validate lat: -PI/2 (exact)");

    // Over boundary (wrap)
    double overLat = PI / 2.0 + 0.1; // slightly over north pole
    double expected = PI - overLat;   // should reflect
    ASSERT_NEAR(NavValidateLatitude(overLat), expected, 1e-12, "Validate lat: over north pole");

    double underLat = -PI / 2.0 - 0.1; // slightly under south pole
    double expectedUnder = -PI - underLat;
    ASSERT_NEAR(NavValidateLatitude(underLat), expectedUnder, 1e-12, "Validate lat: under south pole");
}

void test_NavValidateLongitude() {
    std::cout << "\n--- NavValidateLongitude ---\n";

    ASSERT_NEAR(NavValidateLongitude(0.0), 0.0, 1e-15, "Validate lon: 0");
    ASSERT_NEAR(NavValidateLongitude(PI), PI, 1e-15, "Validate lon: PI (exact)");
    ASSERT_NEAR(NavValidateLongitude(-PI), -PI, 1e-15, "Validate lon: -PI (exact)");

    // Wrap over
    double overLon = PI + 0.5;
    double expected = overLon - 2.0 * PI;
    ASSERT_NEAR(NavValidateLongitude(overLon), expected, 1e-12, "Validate lon: over PI");

    double underLon = -PI - 0.5;
    double expectedUnder = underLon + 2.0 * PI;
    ASSERT_NEAR(NavValidateLongitude(underLon), expectedUnder, 1e-12, "Validate lon: under -PI");
}

void test_MulMatVec3() {
    std::cout << "\n--- MulMatVec3 ---\n";

    // Identity matrix
    {
        double identity[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
        double vin[3] = {1.0, 2.0, 3.0};
        double vout[3];
        MulMatVec3(identity, vin, vout);
        ASSERT_NEAR(vout[0], 1.0, 1e-15, "Identity * [1,2,3]: x=1");
        ASSERT_NEAR(vout[1], 2.0, 1e-15, "Identity * [1,2,3]: y=2");
        ASSERT_NEAR(vout[2], 3.0, 1e-15, "Identity * [1,2,3]: z=3");
    }

    // Known rotation (90 deg around Z)
    {
        double rot90z[3][3] = {{0,-1,0},{1,0,0},{0,0,1}};
        double vin[3] = {1.0, 0.0, 0.0};
        double vout[3];
        MulMatVec3(rot90z, vin, vout);
        ASSERT_NEAR(vout[0], 0.0, 1e-15, "Rot90Z * [1,0,0]: x=0");
        ASSERT_NEAR(vout[1], 1.0, 1e-15, "Rot90Z * [1,0,0]: y=1");
        ASSERT_NEAR(vout[2], 0.0, 1e-15, "Rot90Z * [1,0,0]: z=0");
    }

    // Scaling matrix
    {
        double scale[3][3] = {{2,0,0},{0,3,0},{0,0,4}};
        double vin[3] = {1.0, 1.0, 1.0};
        double vout[3];
        MulMatVec3(scale, vin, vout);
        ASSERT_NEAR(vout[0], 2.0, 1e-15, "Scale * [1,1,1]: x=2");
        ASSERT_NEAR(vout[1], 3.0, 1e-15, "Scale * [1,1,1]: y=3");
        ASSERT_NEAR(vout[2], 4.0, 1e-15, "Scale * [1,1,1]: z=4");
    }
}

// ========================================================================
// SECTION 2: Geometric Helper Tests
// ========================================================================

void test_areAlmostEqual() {
    std::cout << "\n--- areAlmostEqual ---\n";

    ASSERT_TRUE(areAlmostEqual(0.0, 0.0), "0.0 == 0.0");
    ASSERT_TRUE(areAlmostEqual(1.0, 1.0), "1.0 == 1.0");
    ASSERT_TRUE(areAlmostEqual(1.0, 1.0 + 1e-8), "1.0 ~ 1.0+1e-8");
    ASSERT_FALSE(areAlmostEqual(1.0, 1.0 + 1e-5), "1.0 != 1.0+1e-5");
    ASSERT_TRUE(areAlmostEqual(-5.0, -5.0), "-5.0 == -5.0");
    ASSERT_FALSE(areAlmostEqual(0.0, 1e-5), "0.0 != 1e-5");
}

void test_getDistSq() {
    std::cout << "\n--- getDistSq ---\n";

    SPointNE a = {0.0f, 0.0f};
    SPointNE b = {3.0f, 4.0f};
    ASSERT_NEAR(getDistSq(a, b), 25.0, 1e-6, "Distance squared (3,4) = 25");

    SPointNE c = {1.0f, 1.0f};
    ASSERT_NEAR(getDistSq(a, c), 2.0, 1e-6, "Distance squared (1,1) = 2");

    // Same point
    ASSERT_NEAR(getDistSq(a, a), 0.0, 1e-10, "Distance squared same point = 0");
}

void test_getDistToSegmentSquared() {
    std::cout << "\n--- getDistToSegmentSquared ---\n";

    // Point on segment
    SPointNE a = {0.0f, 0.0f};
    SPointNE b = {10.0f, 0.0f};
    SPointNE p_on = {5.0f, 0.0f};
    ASSERT_NEAR(getDistToSegmentSquared(p_on, a, b), 0.0, 1e-6, "Point on segment = 0");

    // Point perpendicular
    SPointNE p_perp = {5.0f, 3.0f};
    ASSERT_NEAR(getDistToSegmentSquared(p_perp, a, b), 9.0, 1e-4, "Point 3m perpendicular = 9");

    // Point beyond endpoint
    SPointNE p_beyond = {15.0f, 0.0f};
    ASSERT_NEAR(getDistToSegmentSquared(p_beyond, a, b), 25.0, 1e-4, "Point beyond endpoint = 25");

    // Zero-length segment
    ASSERT_NEAR(getDistToSegmentSquared(p_perp, a, a), 34.0, 1e-4, "Zero-length segment: dist to point");
}

void test_orientation() {
    std::cout << "\n--- orientation ---\n";

    SPointNE p = {0.0f, 0.0f};
    SPointNE q = {1.0f, 0.0f};

    // Counter-clockwise
    SPointNE r_ccw = {1.0f, 1.0f};
    ASSERT_TRUE(orientation(p, q, r_ccw) == 2, "CCW orientation");

    // Clockwise
    SPointNE r_cw = {1.0f, -1.0f};
    ASSERT_TRUE(orientation(p, q, r_cw) == 1, "CW orientation");

    // Collinear
    SPointNE r_col = {2.0f, 0.0f};
    ASSERT_TRUE(orientation(p, q, r_col) == 0, "Collinear orientation");
}

void test_doSegmentsIntersect() {
    std::cout << "\n--- doSegmentsIntersect ---\n";

    // Crossing segments
    SPointNE a1 = {0.0f, 0.0f}, b1 = {10.0f, 10.0f};
    SPointNE a2 = {0.0f, 10.0f}, b2 = {10.0f, 0.0f};
    ASSERT_TRUE(doSegmentsIntersect(a1, b1, a2, b2), "X-crossing segments");

    // Parallel non-intersecting
    SPointNE c1 = {0.0f, 0.0f}, d1 = {10.0f, 0.0f};
    SPointNE c2 = {0.0f, 1.0f}, d2 = {10.0f, 1.0f};
    ASSERT_FALSE(doSegmentsIntersect(c1, d1, c2, d2), "Parallel segments");

    // T-junction (endpoint on segment)
    SPointNE t1 = {0.0f, 0.0f}, t2 = {10.0f, 0.0f};
    SPointNE t3 = {5.0f, 0.0f}, t4 = {5.0f, 5.0f};
    ASSERT_TRUE(doSegmentsIntersect(t1, t2, t3, t4), "T-junction");

    // Non-intersecting
    SPointNE n1 = {0.0f, 0.0f}, n2 = {1.0f, 0.0f};
    SPointNE n3 = {5.0f, 5.0f}, n4 = {6.0f, 5.0f};
    ASSERT_FALSE(doSegmentsIntersect(n1, n2, n3, n4), "Far apart segments");

    // Shared endpoint
    SPointNE s1 = {0.0f, 0.0f}, s2 = {5.0f, 5.0f};
    SPointNE s3 = {5.0f, 5.0f}, s4 = {10.0f, 0.0f};
    ASSERT_TRUE(doSegmentsIntersect(s1, s2, s3, s4), "Shared endpoint");
}

// ========================================================================
// SECTION 3: Robustness Tests
// ========================================================================

void test_robustness_null_output_ptrs() {
    std::cout << "\n--- Robustness: Null Output Pointers ---\n";

    SPointNE poly[] = {{0,0},{10,0},{10,10},{0,10}};
    SPointNE pt = {5.0f, 5.0f};

    // Should not crash — just return silently
    isInsidePolygonNED(poly, 4, pt, 0.0f, nullptr, nullptr);
    std::cout << "[PASS] isInsidePolygonNED with null outResult/resultState (no crash)" << std::endl;
    g_tests_passed++;

    doesLineIntersectPolygonNED(poly, 4, pt, 0.0f, 10.0f, nullptr, nullptr);
    std::cout << "[PASS] doesLineIntersectPolygonNED with null out-params (no crash)" << std::endl;
    g_tests_passed++;
}

void test_robustness_degenerate_polygons() {
    std::cout << "\n--- Robustness: Degenerate Polygons ---\n";

    uint8_t result = 0;
    uint8_t state = 0;

    // All vertices at same point
    SPointNE degenerate[] = {{5.0f, 5.0f}, {5.0f, 5.0f}, {5.0f, 5.0f}};
    SPointNE pt = {5.0f, 5.0f};
    isInsidePolygonNED(degenerate, 3, pt, 0.0f, &result, &state);
    ASSERT_TRUE(state == EIsInsideResult::IS_INSIDE_OK, "Degenerate polygon (same point): state OK");
    // Point is on vertex -> should report inside/collision
    ASSERT_TRUE(result == 1, "Point on degenerate vertex: collision");

    // Collinear polygon (all points on a line)
    SPointNE collinear[] = {{0.0f, 0.0f}, {5.0f, 0.0f}, {10.0f, 0.0f}};
    SPointNE pt_online = {3.0f, 0.0f};
    isInsidePolygonNED(collinear, 3, pt_online, 0.0f, &result, &state);
    ASSERT_TRUE(state == EIsInsideResult::IS_INSIDE_OK, "Collinear polygon: state OK (no crash)");
    // On boundary of a zero-area polygon
    std::cout << "[PASS] Collinear polygon handled without crash" << std::endl;
    g_tests_passed++;
}

void test_robustness_extreme_coords() {
    std::cout << "\n--- Robustness: Extreme Coordinates ---\n";

    // Very large coordinates
    SPointNE large_poly[] = {{1e6f, 1e6f}, {1e6f, 2e6f}, {2e6f, 2e6f}, {2e6f, 1e6f}};
    SPointNE large_pt = {1.5e6f, 1.5e6f};
    uint8_t result = 0, state = 0;
    isInsidePolygonNED(large_poly, 4, large_pt, 0.0f, &result, &state);
    ASSERT_TRUE(state == EIsInsideResult::IS_INSIDE_OK, "Large coords: state OK");
    ASSERT_TRUE(result == 1, "Large coords: point inside");

    // Very small polygon
    SPointNE tiny_poly[] = {{0.0f, 0.0f}, {0.0f, 1e-4f}, {1e-4f, 1e-4f}, {1e-4f, 0.0f}};
    SPointNE tiny_pt = {5e-5f, 5e-5f};
    isInsidePolygonNED(tiny_poly, 4, tiny_pt, 0.0f, &result, &state);
    ASSERT_TRUE(state == EIsInsideResult::IS_INSIDE_OK, "Tiny polygon: state OK");
    ASSERT_TRUE(result == 1, "Tiny polygon: point inside");
}

void test_robustness_coord_conversions_poles() {
    std::cout << "\n--- Robustness: Pole Singularities ---\n";

    // Exact north pole
    {
        SPointGeo northPole = {90.0, 0.0, 0.0};
        SPointECEF ecef = GeoToEcef(northPole);
        SPointGeo recovered = EcefToGeo(ecef);
        ASSERT_NEAR(recovered.latitudeDeg, 90.0, 0.01, "North pole round-trip: lat=90");
        // Longitude is undefined at poles, so we don't check it
    }

    // Exact south pole
    {
        SPointGeo southPole = {-90.0, 0.0, 0.0};
        SPointECEF ecef = GeoToEcef(southPole);
        SPointGeo recovered = EcefToGeo(ecef);
        ASSERT_NEAR(recovered.latitudeDeg, -90.0, 0.01, "South pole round-trip: lat=-90");
    }

    // Near-pole point (should be numerically stable)
    {
        SPointGeo nearPole = {89.9999, 45.0, 0.0};
        SPointECEF ecef = GeoToEcef(nearPole);
        SPointGeo recovered = EcefToGeo(ecef);
        ASSERT_NEAR(recovered.latitudeDeg, 89.9999, 1e-4, "Near-pole: lat preserved");
        ASSERT_NEAR(recovered.longitudeDeg, 45.0, 1e-4, "Near-pole: lon preserved");
    }
}

void test_robustness_ned_at_pole_origin() {
    std::cout << "\n--- Robustness: NED with Pole Origin ---\n";

    // Origin at north pole - the NED frame is degenerate but should not crash
    double poleLat = 89.99; // near-pole (exact 90 has degenerate rotation matrix)
    double poleLon = 0.0;
    double poleAlt = 0.0;

    SPointGeo target = {89.98, 0.01, 0.0};
    SPointGeo poleOrigin = {poleLat, poleLon, poleAlt};
    SPointNED ned;
    GeoToNed(poleOrigin, target, &ned);

    // Should produce some finite result without NaN
    ASSERT_TRUE(!std::isnan(ned.north), "NED at pole origin: north not NaN");
    ASSERT_TRUE(!std::isnan(ned.east), "NED at pole origin: east not NaN");
    ASSERT_TRUE(!std::isnan(ned.down), "NED at pole origin: down not NaN");
    ASSERT_TRUE(std::isfinite(ned.north), "NED at pole origin: north finite");
    ASSERT_TRUE(std::isfinite(ned.east), "NED at pole origin: east finite");
    ASSERT_TRUE(std::isfinite(ned.down), "NED at pole origin: down finite");
}

void test_robustness_line_boundary_azimuths() {
    std::cout << "\n--- Robustness: Line Boundary Azimuths ---\n";

    SPointNE poly[] = {{0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};
    SPointNE pt_outside = {-5.0f, 5.0f};
    uint8_t result = 0, state = 0;

    // Azimuth exactly 0 (North)
    doesLineIntersectPolygonNED(poly, 4, pt_outside, 0.0f, 20.0f, &result, &state);
    ASSERT_TRUE(state == ELineIntersectResult::LINE_INTERSECT_OK, "Azimuth 0: state OK");

    // Azimuth 360 (same as 0)
    doesLineIntersectPolygonNED(poly, 4, pt_outside, 360.0f, 20.0f, &result, &state);
    ASSERT_TRUE(state == ELineIntersectResult::LINE_INTERSECT_OK, "Azimuth 360: state OK");

    // Azimuth 90 (East) - line from (-5, -5) going East, should cross polygon at east=0
    SPointNE pt_hits = {5.0f, -5.0f}; // north=5, east=-5 (west of polygon)
    doesLineIntersectPolygonNED(poly, 4, pt_hits, 90.0f, 20.0f, &result, &state);
    ASSERT_TRUE(state == ELineIntersectResult::LINE_INTERSECT_OK && result == 1, "Azimuth 90 hitting polygon: collision");

    // Very small length (epsilon)
    doesLineIntersectPolygonNED(poly, 4, pt_outside, 90.0f, 0.001f, &result, &state);
    ASSERT_TRUE(state == ELineIntersectResult::LINE_INTERSECT_OK && result == 0, "Tiny line outside: no collision");
}

// ========================================================================
// MAIN
// ========================================================================

int main() {
    // --- Section 0: API Version ---
    test_api_version();

    // --- Section 1: Coordinate Conversions ---
    test_GeoToEcef_known_values();
    test_EcefToGeo_known_values();
    test_GeoToEcef_roundtrip();
    test_NedRoundTrip();
    test_EcefToNed_NedToEcef_roundtrip();
    test_NavValidateLatitude();
    test_NavValidateLongitude();
    test_MulMatVec3();

    // --- Section 2: Geometric Helpers ---
    test_areAlmostEqual();
    test_getDistSq();
    test_getDistToSegmentSquared();
    test_orientation();
    test_doSegmentsIntersect();

    // --- Section 3: Robustness ---
    test_robustness_null_output_ptrs();
    test_robustness_degenerate_polygons();
    test_robustness_extreme_coords();
    test_robustness_coord_conversions_poles();
    test_robustness_ned_at_pole_origin();
    test_robustness_line_boundary_azimuths();

    // --- Summary ---
    std::cout << "\n=================================\n";
    std::cout << "COORDS & ROBUSTNESS SUMMARY: Passed: " << g_tests_passed << ", Failed: " << g_tests_failed << std::endl;
    std::cout << "=================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}

#include "api_functions.h"
#include "cov_spy.h"
#include "test_utils.h"
#include "geometric_functions.h"

#include <iostream>
#include <fstream> // Required for file logging
#include <sstream> // Required for string building
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>

// --- Global Log File ---
std::ofstream g_logFile("test_results_geo.log");

// --- Mini Test Framework ---
int g_tests_passed = 0;
int g_tests_failed = 0;

// Helper struct to simplify API outputs for testing
struct ApiResult {
    uint8_t isCollision;
    uint8_t state;
};

// --- Serialization Helpers for Logging ---
std::string PointToStr(const SPointNE& p) {
    std::stringstream ss;
    ss << p.north << "," << p.east;
    return ss.str();
}

std::string PolyToStr(const SPointNE* poly, uint16_t count) {
    if (!poly || count == 0) return "EMPTY";
    std::stringstream ss;
    for (uint16_t i = 0; i < count; ++i) {
        ss << poly[i].north << "," << poly[i].east;
        if (i < count - 1) ss << ";";
    }
    return ss.str();
}

// --- API Wrappers ---
ApiResult CallIsInside(const SPointNE* poly, uint16_t count, const SPointNE& pt, float rad) {
    uint8_t res = false;
    uint8_t state = EIsInsideResult::IS_INSIDE_OK;
    isInsidePolygon(poly, count, pt, rad, &res, &state);
    return { res, state };
}

ApiResult CallIntersect(const SPointNE* poly, uint16_t count, const SPointNE& pt, float az, float len) {
    uint8_t res = false;
    uint8_t state = ELineIntersectResult::LINE_INTERSECT_OK;
    doesLineIntersectPolygon(poly, count, pt, az, len, &res, &state);
    return { res, state };
}

// --- Test Runners with Logging ---

// 1. Runner for Circle/Point Tests
void RunTest_Circle(const std::string& testName, const SPointNE* poly, uint16_t count, SPointNE pt, float rad, bool expectCollision) {
    // A. Run API
    ApiResult result = CallIsInside(poly, count, pt, rad);

    // B. Determine actual boolean (True only if State is OK AND Collision is True)
    bool actualCollision = (result.state == EIsInsideResult::IS_INSIDE_OK && result.isCollision);
    bool passed = (actualCollision == expectCollision);

    // C. Console Output
    if (passed) {
        std::cout << "[PASS] " << testName << std::endl;
        g_tests_passed++;
    }
    else {
        std::cout << "[FAIL] " << testName << " | Expected: " << expectCollision << ", Got: " << actualCollision << std::endl;
        g_tests_failed++;
    }

    // D. Log to File (Format: Name|type|Poly|Pt|Rad|Dummy|Exp|Act)
    if (g_logFile.is_open()) {
        g_logFile << testName << "|"
            << "circle" << "|"
            << PolyToStr(poly, count) << "|"
            << PointToStr(pt) << "|"
            << rad << "|"
            << "0" << "|" // Dummy V2
            << (expectCollision ? "1" : "0") << "|"
            << (actualCollision ? "1" : "0") << "\n";
    }
}

// 2. Runner for Line Intersection Tests
void RunTest_Line(const std::string& testName, const SPointNE* poly, uint16_t count, SPointNE pt, float az, float len, bool expectCollision) {
    // A. Run API
    ApiResult result = CallIntersect(poly, count, pt, az, len);

    // B. Determine actual
    bool actualCollision = (result.state == ELineIntersectResult::LINE_INTERSECT_OK && result.isCollision);
    bool passed = (actualCollision == expectCollision);

    // C. Console Output
    if (passed) {
        std::cout << "[PASS] " << testName << std::endl;
        g_tests_passed++;
    }
    else {
        std::cout << "[FAIL] " << testName << " | Expected: " << expectCollision << ", Got: " << actualCollision << std::endl;
        g_tests_failed++;
    }

    // D. Log to File (Format: Name|type|Poly|Pt|Az|Len|Exp|Act)
    if (g_logFile.is_open()) {
        g_logFile << testName << "|"
            << "line" << "|"
            << PolyToStr(poly, count) << "|"
            << PointToStr(pt) << "|"
            << az << "|"
            << len << "|"
            << (expectCollision ? "1" : "0") << "|"
            << (actualCollision ? "1" : "0") << "\n";
    }
}

// 3. Macro for Validation Tests (No Geometry Logging needed)
#define ASSERT_ERROR_STATE(call, expectedState, msg) \
    { \
        ApiResult res = call; \
        if (res.state == expectedState) { \
            std::cout << "[PASS] " << msg << std::endl; \
            g_tests_passed++; \
        } else { \
            std::cout << "[FAIL] " << msg << " | Expected State: " << (int)expectedState << ", Got: " << (int)res.state << std::endl; \
            g_tests_failed++; \
        } \
    }

// --- Test Data ---
SPointNE square_polygon[] = {
    {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}
};
uint16_t square_size = 4;

SPointNE u_shape_pts[] = {
    {0.0f, 0.0f},   {10.0f, 0.0f},  {10.0f, 10.0f}, {7.0f, 10.0f},
    {7.0f, 3.0f},   {3.0f, 3.0f},   {3.0f, 10.0f},  {0.0f, 10.0f}
};
uint16_t u_shape_size = sizeof(u_shape_pts) / sizeof(SPointNE);

SPointNE triangle_pts[] = {
    {0.0f, 0.0f}, {10.0f, 2.0f}, {0.0f, 4.0f}
};
uint16_t triangle_size = sizeof(triangle_pts) / sizeof(SPointNE);
// --- Tests ---

void test_is_inside() {
    std::cout << "\n--- Testing isInsidePolygon (Polygon is Obstacle) ---\n";

    // 1. Inside -> Collision
    RunTest_Circle("Inside Strict", square_polygon, square_size, { 5.0f, 5.0f }, 0.0f, true);

    // 2. Outside -> Safe
    RunTest_Circle("Outside Strict", square_polygon, square_size, { 20.0f, 5.0f }, 0.0f, false);

    // 3. On Boundary -> Collision
    RunTest_Circle("On Boundary", square_polygon, square_size, { 0.0f, 5.0f }, 0.0f, true);

    // 4. Near Boundary (Radius Conflict) -> Collision
    RunTest_Circle("Outside Radius Hit", square_polygon, square_size, { -1.0f, 5.0f }, 2.0f, true);

    // 5. Safe Radius -> Safe
    RunTest_Circle("Outside Safe Radius", square_polygon, square_size, { -5.0f, 5.0f }, 2.0f, false);

    // 6. Optimization Check -> Safe
    RunTest_Circle("Far Outside Opt", square_polygon, square_size, { 20.0f, 20.0f }, 1.0f, false);

    // 7. Input Validation (Edge Cases - No Visual Log)
    ASSERT_ERROR_STATE(CallIsInside(nullptr, 0, { 5,5 }, 0), EIsInsideResult::IS_INSIDE_POLYGON_IS_NULL_PTR, "Null Poly Check");
    ASSERT_ERROR_STATE(CallIsInside(square_polygon, 2, { 5,5 }, 0), EIsInsideResult::IS_INSIDE_POLYGON_WITH_LESS_THAN_3_POINTS, "Small Poly Check");

    // 8. Point on East Edge (Right side)
    // Point {5.0, 10.0} is on the East edge of the 10x10 square.
    RunTest_Circle("Point on East Edge", square_polygon, square_size, { 5.0f, 10.0f }, 0.0f, true);
    // 9. Concave "Bay" Area (Strictly Outside) -> Safe
    // Point (5, 8) is inside the bounding box, but in the empty space of the U-shape.
    RunTest_Circle("Concave Bay (Safe)", u_shape_pts, u_shape_size, { 5.0f, 8.0f }, 0.0f, false);

    // 10. Concave "Bay" Wall Hit (Radius) -> Collision
    // Point (5, 8) is 2.0m away from the walls at x=3 and x=7. Radius 2.1 hits.
    RunTest_Circle("Concave Bay (Hit)", u_shape_pts, u_shape_size, { 5.0f, 8.0f }, 2.1f, true);

    // 11. Concave "Squeeze" (Radius Safe) -> Safe
    // Radius 1.9 fits in the 2.0m gap.
    RunTest_Circle("Concave Squeeze", u_shape_pts, u_shape_size, { 5.0f, 8.0f }, 1.9f, false);

    // 12. Sharp Vertex (Exact Tip) -> Collision
    // Testing mathematical precision on acute angles.
    RunTest_Circle("Sharp Vertex Tip", triangle_pts, triangle_size, { 10.0f, 2.0f }, 0.0f, true);

    // 13. Near Sharp Vertex (Radius) -> Collision
    // Point is just off the tip, but radius catches it.
    RunTest_Circle("Near Sharp Tip", triangle_pts, triangle_size, { 10.1f, 2.0f }, 0.2f, true);
}

void test_intersection() {
    std::cout << "\n--- Testing doesLineIntersectPolygon ---\n";

    // 1. Crossing Out -> Collision
    RunTest_Line("Ray Inside Out", square_polygon, square_size, { 5,5 }, 0.0f, 100.0f, true);

    // 2. Contained Line -> Collision
    RunTest_Line("Ray Contained", square_polygon, square_size, { 5,5 }, 0.0f, 1.0f, true);

    // 3. Outside Parallel -> Safe
    RunTest_Line("Ray Outside Parallel", square_polygon, square_size, { -5,-0.1f }, 0.0f, 10.0f, false);

    // 3. On Boundry Parallel -> Collision
    RunTest_Line("On Boundry Parallel", square_polygon, square_size, { -5, 0.0f }, 0.0f, 10.0f, true);

    // 4. Crossing In -> Collision
    RunTest_Line("Ray Crossing In", square_polygon, square_size, { 5,-5 }, 90.0f, 20.0f, true);

    // 5. Input Validation (No Visual Log)
    ASSERT_ERROR_STATE(CallIntersect(nullptr, 0, { 0,0 }, 0.0f, 10.0f), ELineIntersectResult::LINE_INTERSECT_POLYGON_IS_NULL_PTR, "Null Poly Line");
    ASSERT_ERROR_STATE(CallIntersect(square_polygon, square_size, { -5.0f, 5.0f }, 0.0f, 0.0f), ELineIntersectResult::LINE_INTERSECT_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO, "Zero Len Line");
    ASSERT_ERROR_STATE(CallIntersect(square_polygon, 2, { 0,0 }, 0.0f, 10.0f),ELineIntersectResult::LINE_INTERSECT_POLYGON_WITH_LESS_THAN_3_POINTS, "Hit COV 2 (Small Poly Count)");

    // 6. Ray Above Concave Bay -> Safe
    // Ray travels over the "dip" in the U-shape without entering the solid part.
    RunTest_Line("Ray Above Bay", u_shape_pts, u_shape_size, { 5.0f, 15.0f }, 180.0f, 4.0f, false);

    // 7. Hit Inner Floor (Concave) -> Collision
    // Ray enters the bay and hits the "floor" of the U-shape.
    RunTest_Line("Hit Inner Floor", u_shape_pts, u_shape_size, { 5.0f, 5.0f }, 180.0f, 5.0f, true);

    // 8. Threading the Needle -> Safe/False
    // Ray passes through the opening of the U but stops before hitting the back wall.
    // It enters the bounding box but crosses no edges.
    RunTest_Line("Thread Needle", u_shape_pts, u_shape_size, { 5.0f, 12.0f }, 180.0f, 6.0f, false);

    // 9. Collinear Overlap -> Collision
    // Ray lies exactly ON the edge of the square.
    RunTest_Line("Collinear Overlap", square_polygon, square_size, { -1.0f, 0.0f }, 90.0f, 12.0f, false);

    // 10. Grazing Vertex -> Collision
    // Ray touches exactly the corner point (0,10) but continues outside.
    // Should be detected as collision in most safety-critical systems.
    RunTest_Line("Grazing Vertex", square_polygon, square_size, { -5.0f, 10.0f }, 90.0f, 10.0f, false);
}

// --- Edge Case Tests: isInsidePolygon ---
void test_is_inside_edge_cases() {
    std::cout << "\n--- Edge Cases: isInsidePolygon ---\n";

    // 1. Point exactly on a vertex (corner of square)
    RunTest_Circle("Exact Vertex (0,0)", square_polygon, square_size, { 0.0f, 0.0f }, 0.0f, true);
    RunTest_Circle("Exact Vertex (10,10)", square_polygon, square_size, { 10.0f, 10.0f }, 0.0f, true);

    // 2. Radius exactly equals distance to nearest edge (boundary condition)
    // Point at (-2, 5), distance to west edge (east=0) is 2.0m
    RunTest_Circle("Radius == Distance (boundary)", square_polygon, square_size, { -2.0f, 5.0f }, 2.0f, false);

    // 3. Counter-clockwise wound polygon (same square, reversed vertex order)
    SPointNE ccw_square[] = { {10.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 10.0f}, {0.0f, 0.0f} };
    RunTest_Circle("CCW Polygon Inside", ccw_square, 4, { 5.0f, 5.0f }, 0.0f, true);
    RunTest_Circle("CCW Polygon Outside", ccw_square, 4, { 20.0f, 5.0f }, 0.0f, false);

    // 4. Self-intersecting polygon (bowtie shape)
    SPointNE bowtie[] = { {0.0f, 0.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}, {0.0f, 10.0f} };
    // Center of bowtie (5,5) is the crossing point - ray casting behavior is implementation-defined
    RunTest_Circle("Bowtie Center", bowtie, 4, { 5.0f, 5.0f }, 0.0f, true);

    // 5. Polygon entirely in negative coordinate space
    SPointNE neg_square[] = { {-10.0f, -10.0f}, {-10.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, -10.0f} };
    RunTest_Circle("Negative Space Inside", neg_square, 4, { -5.0f, -5.0f }, 0.0f, true);
    RunTest_Circle("Negative Space Outside", neg_square, 4, { 5.0f, 5.0f }, 0.0f, false);

    // 6. Very thin horizontal polygon (1m tall, 100m wide)
    SPointNE thin_poly[] = { {0.0f, 0.0f}, {0.0f, 100.0f}, {1.0f, 100.0f}, {1.0f, 0.0f} };
    RunTest_Circle("Thin Poly Inside", thin_poly, 4, { 0.5f, 50.0f }, 0.0f, true);
    RunTest_Circle("Thin Poly Radius Miss", thin_poly, 4, { -1.0f, 50.0f }, 0.5f, false);
    RunTest_Circle("Thin Poly Radius Hit", thin_poly, 4, { -0.5f, 50.0f }, 0.6f, true);

    // 7. L-shaped polygon (concave, different from U-shape)
    SPointNE L_shape[] = { {0.0f,0.0f}, {0.0f,10.0f}, {5.0f,10.0f}, {5.0f,5.0f}, {10.0f,5.0f}, {10.0f,0.0f} };
    RunTest_Circle("L-Shape Inside Arm", L_shape, 6, { 2.0f, 8.0f }, 0.0f, true);
    RunTest_Circle("L-Shape Inside Base", L_shape, 6, { 8.0f, 2.0f }, 0.0f, true);
    RunTest_Circle("L-Shape Outside Notch", L_shape, 6, { 8.0f, 8.0f }, 0.0f, false);

    // 8. Point at the origin with polygon centered around it
    SPointNE centered[] = { {-5.0f, -5.0f}, {-5.0f, 5.0f}, {5.0f, 5.0f}, {5.0f, -5.0f} };
    RunTest_Circle("Origin Centered Inside", centered, 4, { 0.0f, 0.0f }, 0.0f, true);

    // 9. Pentagon (non-axis-aligned edges)
    SPointNE pentagon[] = { {5.0f, 0.0f}, {1.5f, 4.8f}, {3.1f, 8.0f}, {6.9f, 8.0f}, {8.5f, 4.8f} };
    RunTest_Circle("Pentagon Inside", pentagon, 5, { 5.0f, 5.0f }, 0.0f, true);
    RunTest_Circle("Pentagon Outside", pentagon, 5, { 0.0f, 0.0f }, 0.0f, false);
}

// --- Edge Case Tests: doesLineIntersectPolygon ---
void test_intersection_edge_cases() {
    std::cout << "\n--- Edge Cases: doesLineIntersectPolygon ---\n";

    // 1. Negative azimuth (equivalent to 270 degrees = West)
    // Point at (5, 15) going West (-90 = 270) should hit east edge at east=10
    RunTest_Line("Negative Azimuth -90", square_polygon, square_size, { 5.0f, 15.0f }, -90.0f, 10.0f, true);

    // 2. Azimuth > 360 (wrapped, 450 = 90 = East)
    RunTest_Line("Azimuth 450 (wrapped)", square_polygon, square_size, { 5.0f, -5.0f }, 450.0f, 20.0f, true);

    // 3. Line stops just BEFORE reaching polygon
    // Point at (5, -5), going East (90), length 4.9 -> endpoint at east=-0.1 (just before west edge at 0)
    RunTest_Line("Stops Before Polygon", square_polygon, square_size, { 5.0f, -5.0f }, 90.0f, 4.9f, false);

    // 4. Line reaches exactly TO the polygon edge
    // Point at (5, -5), going East (90), length 5.0 -> endpoint exactly at east=0 (west edge)
    RunTest_Line("Reaches Edge Exactly", square_polygon, square_size, { 5.0f, -5.0f }, 90.0f, 5.0f, true);

    // 5. Line starting on a vertex of the polygon
    RunTest_Line("Start On Vertex", square_polygon, square_size, { 0.0f, 0.0f }, 45.0f, 5.0f, true);

    // 6. Line starting on an edge (south edge, between vertices)
    RunTest_Line("Start On Edge", square_polygon, square_size, { 0.0f, 5.0f }, 0.0f, 2.0f, true);

    // 7. Diagonal line (45 degrees) that just misses the NE corner
    // From (-1, 11) going at 45 degrees NE, short line that passes above the corner (10,10)
    RunTest_Line("Diagonal Miss Corner", square_polygon, square_size, { -1.0f, 11.0f }, 45.0f, 5.0f, false);

    // 8. Diagonal line (45 degrees) that hits the polygon
    // From (5, -3) going at 45 degrees NE -> hits the west edge
    RunTest_Line("Diagonal Hit", square_polygon, square_size, { 5.0f, -3.0f }, 45.0f, 10.0f, true);

    // 9. Very long line (potential float precision issues)
    RunTest_Line("Very Long Line", square_polygon, square_size, { 5.0f, -10000.0f }, 90.0f, 20000.0f, true);

    // 10. Line going South (180 degrees) from above polygon
    RunTest_Line("South Into Polygon", square_polygon, square_size, { 15.0f, 5.0f }, 180.0f, 10.0f, true);

    // 11. Line going South (180 degrees) but too short to reach
    RunTest_Line("South Too Short", square_polygon, square_size, { 15.0f, 5.0f }, 180.0f, 4.9f, false);

    // 12. Line completely outside going away from polygon
    RunTest_Line("Going Away", square_polygon, square_size, { -5.0f, 5.0f }, 180.0f, 100.0f, false);

    // 13. Line entering and exiting polygon (two edge crossings)
    // From (-5, 5) going East (90), passes through polygon (enters at east=0, exits at east=10)
    RunTest_Line("Enter And Exit", square_polygon, square_size, { -5.0f, 5.0f }, 90.0f, 100.0f, false);

    // 14. Line through concave notch of L-shape
    SPointNE L_shape[] = { {0.0f,0.0f}, {0.0f,10.0f}, {5.0f,10.0f}, {5.0f,5.0f}, {10.0f,5.0f}, {10.0f,0.0f} };
    // From (7, 7) which is in the notch (outside), going South (180) into the base
    RunTest_Line("L-Shape Notch to Base", L_shape, 6, { 7.0f, 7.0f }, 180.0f, 5.0f, true);
    // From (7, 7) going North - should be safe (away from polygon)
    RunTest_Line("L-Shape Notch Away", L_shape, 6, { 7.0f, 7.0f }, 0.0f, 5.0f, false);

    // 15. Epsilon-length line inside polygon
    RunTest_Line("Epsilon Inside", square_polygon, square_size, { 5.0f, 5.0f }, 0.0f, 0.001f, true);

    // 16. Line with azimuth 180 (due South) starting above north edge
    RunTest_Line("Due South Hit North Edge", square_polygon, square_size, { 12.0f, 5.0f }, 180.0f, 3.0f, true);

    // 17. Negative length should error
    ASSERT_ERROR_STATE(CallIntersect(square_polygon, square_size, { 5.0f, 5.0f }, 0.0f, -5.0f), ELineIntersectResult::LINE_INTERSECT_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO, "Negative Len Line");
}

void verify_full_coverage(int total_expected, ECovFuncID funcID, std::string func_name) {
#if defined(_DEBUG) || !defined(NDEBUG)
    std::cout << "\n--- Coverage Verification ---\n";

    bool* coverage_map = GetCoverageArray(funcID);

    int missed_count = 0;
    for (int i = 0; i < total_expected; i++) {
        if (!coverage_map[i]) {
            std::cout << "[FAIL] Code Logic at COV_POINT(" << i << ") was NEVER executed!" << std::endl;
            missed_count++;
        }
    }

    if (missed_count == 0) {
        std::cout << "[SUCCESS] 100% Logic Coverage Achieved for " << func_name << "!" << std::endl;
    }
    else {
        std::cout << "[WARNING] In Function " << func_name << " Logic Coverage is NOT 100%. Missed " << missed_count << " blocks." << std::endl;
        g_tests_failed++;
    }
#else
    std::cout << "\n--- Coverage Verification Skipped (Release Mode) ---\n";
#endif
}

int main() {
#if defined(_DEBUG) || !defined(NDEBUG)

#ifdef _WIN32
    ResetCoverage();
#endif


    // 1. Test isInsidePolygon
    test_is_inside();
    test_is_inside_edge_cases();
    verify_full_coverage(11, ECovFuncID::IsInside, "isInsidePolygon");

    // 2. Test doesLineIntersectPolygon
    test_intersection();
    test_intersection_edge_cases();
    verify_full_coverage(7, ECovFuncID::Intersect, "doesLineIntersectPolygon");

    std::cout << "\n---------------------------------\n";
    std::cout << "SUMMARY: Passed: " << g_tests_passed << ", Failed: " << g_tests_failed << std::endl;
    std::cout << "Log saved to: test_results_geo.log" << std::endl;

    if (g_logFile.is_open()) g_logFile.close();

    return (g_tests_failed == 0) ? 0 : 1;
#endif // DEBUG

}


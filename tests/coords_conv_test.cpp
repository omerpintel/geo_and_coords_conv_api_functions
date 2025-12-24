#include "api_functions.h"
#include "cov_spy.h" 
#include "test_utils.h"
#include "coords_conv_functions.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>

// --- Global Log File ---
std::ofstream g_logFile("test_results_wgs84.log");

int g_tests_passed = 0;
int g_tests_failed = 0;

// Tolerances
const double EPSILON_M = 0.01;       // 1 cm tolerance
const double EPSILON_RAD = 1e-7;
const double EPSILON_ALT = 1e-7;

// --- Helpers & Structs ---
const double WGS84_A = 6378137.0;
const double WGS84_B = 6356752.314245;


double DegToRad(double deg) { return deg * (PI / 180.0); }

std::string GeoToStr(const SPointGeo* p) {
    if (!p) return "NULL";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(8) << p->latitude << "," << p->longitude << "," << p->altitude;
    return ss.str();
}

std::string EcefToStr(const SPointECEF* p) {
    if (!p) return "NULL";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << p->x << "," << p->y << "," << p->z;
    return ss.str();
}

std::string NedToStr(const SPointNED* p) {
    if (!p) return "NULL";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << p->north << "," << p->east << "," << p->down;
    return ss.str();
}

// --- API Wrappers ---
SPointECEF CallGeoToEcef(const SPointGeo& input) { return GeoToEcef(&input); }
SPointGeo  CallEcefToGeo(const SPointECEF& input) { return EcefToGeo(&input); }
SPointNED  CallEcefToNed(const double lat, const double lon, const SPointECEF& pt) {
    return EcefToNed(&lat, &lon, &pt);
}

// --- Test Runners ---

void RunTest_GeoToEcef(const std::string& testName, SPointGeo input, SPointECEF expected) {
    SPointECEF actual = CallGeoToEcef(input);
    double diffX = std::abs(actual.x - expected.x);
    double diffY = std::abs(actual.y - expected.y);
    double diffZ = std::abs(actual.z - expected.z);
    bool passed = (diffX < EPSILON_M) && (diffY < EPSILON_M) && (diffZ < EPSILON_M);

    if (passed) { g_tests_passed++; std::cout << "[PASS] " << testName << std::endl; }
    else { g_tests_failed++; std::cout << "[FAIL] " << testName << " Diff: " << diffX << "," << diffY << "," << diffZ << std::endl; }

    if (g_logFile.is_open())
        g_logFile << testName << "|wgs84_conv|" << GeoToStr(&input) << "|" << EcefToStr(&expected) << "|" << EcefToStr(&actual) << "|" << (passed ? "1" : "0") << "\n";
}

void RunTest_EcefToGeo(const std::string& testName, SPointECEF input, SPointGeo expected) {
    SPointGeo actual = CallEcefToGeo(input);
    double diffLat = std::abs(actual.latitude - expected.latitude);
    double diffLon = std::abs(actual.longitude - expected.longitude);
    double diffAlt = std::abs(actual.altitude - expected.altitude);
    bool passed = (diffLat < EPSILON_RAD) && (diffLon < EPSILON_RAD) && (diffAlt < EPSILON_ALT);

    if (passed) { 
        g_tests_passed++; std::cout << "[PASS] " << testName << std::endl; 
    }
    else { 
        g_tests_failed++; std::cout << "[FAIL] " << testName << std::endl; 
    }

    if (g_logFile.is_open())
        g_logFile << testName << "|wgs84_inv|" << EcefToStr(&input) << "|" << GeoToStr(&expected) << "|" << GeoToStr(&actual) << "|" << (passed ? "1" : "0") << "\n";
}

void RunTest_EcefToNed(const std::string& testName, double refLat, double refLon, SPointECEF targetPt, SPointNED expected) {
    SPointNED actual = CallEcefToNed(refLat, refLon, targetPt);

    double diffN = std::abs(actual.north - expected.north);
    double diffE = std::abs(actual.east - expected.east);
    double diffD = std::abs(actual.down - expected.down);
    bool passed = (diffN < EPSILON_M) && (diffE < EPSILON_M) && (diffD < EPSILON_M);

    if (passed) {
        g_tests_passed++;
        std::cout << "[PASS] " << testName << std::endl;
    }
    else {
        g_tests_failed++;
        std::cout << "[FAIL] " << testName << "\n"
            << "      Exp: " << NedToStr(&expected) << "\n"
            << "      Got: " << NedToStr(&actual) << "\n"
            << "      Diff: " << diffN << ", " << diffE << ", " << diffD << std::endl;
    }

    SPointGeo refGeo = { refLat, refLon, 0.0 };
    if (g_logFile.is_open()) {
        g_logFile << testName << "|ecef_to_ned|"
            << GeoToStr(&refGeo) << "|"
            << NedToStr(&expected) << "|"
            << NedToStr(&actual) << "|"
            << (passed ? "1" : "0") << "\n";
    }
}

// --- Main ---
int main() {
#if defined(_DEBUG) || !defined(NDEBUG)
    std::cout << "\n--- Starting Coordinate Tests ---\n";

    SPointGeo p1 = { 0,0,0 }; SPointECEF p1_e = { WGS84_A,0,0 };
    RunTest_GeoToEcef("Equator/PM (Forward)", p1, p1_e);
    RunTest_EcefToGeo("Equator/PM (Inverse)", p1_e, p1);

    std::cout << "\n--- Testing EcefToNed ---\n";

    // 1. Identity Check
    // If Target == Reference, NED should be (0,0,0)
    RunTest_EcefToNed("Identity (Ref=Target)",
        0.0, 0.0,              // Ref: Lat 0, Lon 0
        { WGS84_A, 0.0, 0.0 }, // Target: ECEF of Lat 0, Lon 0
        { 0.0, 0.0, 0.0 }      // Exp NED
    );

    // 2. Up/Down Check
    // Ref: (0,0). Target: (0,0, Alt=100m).
    // At Lat 0, Lon 0 (Ref), the "Up" vector is +X axis.
    // So moving +100 in X is "Up" 100m. 
    // In NED, Up is -Down. So Down should be -100.0
    RunTest_EcefToNed("Upwards 100m",
        0.0, 0.0,
        { WGS84_A + 100.0, 0.0, 0.0 },
        { 0.0, 0.0, -100.0 }
    );

    // 3. North Check (Approximation on Tangent Plane)
    // Ref: (0,0). Target: ECEF(A, 0, 100).
    // At Lat 0, Lon 0: North points to +Z ECEF.
    // So a point at Z=100 relative to Ref is 100m North.
    RunTest_EcefToNed("North 100m",
        0.0, 0.0,
        { WGS84_A, 0.0, 100.0 },
        { 100.0, 0.0, 0.0 }
    );

    // 4. East Check
    // Ref: (0,0). Target: ECEF(A, 100, 0).
    // At Lat 0, Lon 0: East points to +Y ECEF.
    RunTest_EcefToNed("East 100m",
        0.0, 0.0,
        { WGS84_A, 100.0, 0.0 },
        { 0.0, 100.0, 0.0 }
    );

    std::cout << "\nSummary: Passed: " << g_tests_passed << ", Failed: " << g_tests_failed << std::endl;
    if (g_logFile.is_open()) g_logFile.close();
    return (g_tests_failed == 0) ? 0 : 1;
#endif
}
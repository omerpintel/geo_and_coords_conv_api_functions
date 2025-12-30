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
std::ofstream g_logFile("test_results_coords_conv.log");

int g_tests_passed = 0;
int g_tests_failed = 0;

// Tolerances
const double EPSILON_M = 0.05;       // 5 cm tolerance (increased slightly for WGS84 approx)
const double EPSILON_DEG = 1e-7;     // Approx 1cm at equator
const double EPSILON_ALT = 1e-3;

// --- Constants ---
const double WGS84_A = 6378137.0;

// --- Helpers ---

std::string GeoToStr(const SPointGeo* p) {
    if (!p) return "NULL";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(8)
        << p->latitudeDeg << "," << p->longitudeDeg << "," << p->altitude;
    return ss.str();
}

std::string NedToStr(const SPointNED* p) {
    if (!p) return "NULL";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3)
        << p->north << "," << p->east << "," << p->down;
    return ss.str();
}

// --- API Wrappers ---

// UPDATED: Now accepts origin lat/lon
SPointNED CallGeoToNed(double originLat, double originLon, const SPointGeo& input) {
    return GeoToNed(&originLat, &originLon, &input);
}

SPointGeo CallNedToGeo(double originLat, double originLon, const SPointNED& input) {
    return NedToGeo(&originLat, &originLon, &input);
}

// --- Test Runners ---

void RunTest_NedToGeo(const std::string& testName, double orgLat, double orgLon, SPointNED inputNed, SPointGeo expectedGeo) {
    SPointGeo actual = CallNedToGeo(orgLat, orgLon, inputNed);

    double diffLat = std::abs(actual.latitudeDeg - expectedGeo.latitudeDeg);
    double diffLon = std::abs(actual.longitudeDeg - expectedGeo.longitudeDeg);
    double diffAlt = std::abs(actual.altitude - expectedGeo.altitude);

    bool passed = (diffLat < EPSILON_DEG) && (diffLon < EPSILON_DEG) && (diffAlt < EPSILON_ALT);

    if (passed) {
        g_tests_passed++;
        std::cout << "[PASS] " << testName << std::endl;
    }
    else {
        g_tests_failed++;
        std::cout << "[FAIL] " << testName << "\n"
            << "      Exp: " << GeoToStr(&expectedGeo) << "\n"
            << "      Got: " << GeoToStr(&actual) << "\n"
            << "      Diff: " << diffLat << ", " << diffLon << ", " << diffAlt << std::endl;
    }

    if (g_logFile.is_open()) {
        g_logFile << testName << "|ned_to_geo|"
            << NedToStr(&inputNed) << "|"
            << GeoToStr(&expectedGeo) << "|"
            << GeoToStr(&actual) << "|"
            << (passed ? "1" : "0") << "\n";
    }
}

void RunTest_GeoToNed(const std::string& testName, double orgLat, double orgLon, SPointGeo inputGeo, SPointNED expectedNed) {
    // UPDATED: Passing origin to the wrapper
    SPointNED actual = CallGeoToNed(orgLat, orgLon, inputGeo);

    double diffN = std::abs(actual.north - expectedNed.north);
    double diffE = std::abs(actual.east - expectedNed.east);
    double diffD = std::abs(actual.down - expectedNed.down);

    bool passed = (diffN < EPSILON_M) && (diffE < EPSILON_M) && (diffD < EPSILON_M);

    if (passed) {
        g_tests_passed++;
        std::cout << "[PASS] " << testName << std::endl;
    }
    else {
        g_tests_failed++;
        std::cout << "[FAIL] " << testName << "\n"
            << "      Exp: " << NedToStr(&expectedNed) << "\n"
            << "      Got: " << NedToStr(&actual) << "\n"
            << "      Diff: " << diffN << ", " << diffE << ", " << diffD << std::endl;
    }

    if (g_logFile.is_open()) {
        g_logFile << testName << "|geo_to_ned|"
            << GeoToStr(&inputGeo) << "|"
            << NedToStr(&expectedNed) << "|"
            << NedToStr(&actual) << "|"
            << (passed ? "1" : "0") << "\n";
    }
}

// --- Main ---
int main() {
#if defined(_DEBUG) || !defined(NDEBUG)
    std::cout << "\n--- Starting Coordinate Tests (NED <-> Geo) ---\n";

    // Constants for approximation
    // 1 deg lat approx 111,319.49m
    const double DEG_LAT_M = 111319.49;

    // --- Section 1: NED to Geo ---
    std::cout << "\n[NED -> Geo Tests]\n";

    RunTest_NedToGeo("N2G: Identity (0,0)",
        0.0, 0.0,
        { 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0f }
    );

    RunTest_NedToGeo("N2G: North 1 deg",
        0.0, 0.0,
        { DEG_LAT_M, 0.0, 0.0 },
        { 1, 0.0, 977.92 }
    );

    RunTest_NedToGeo("N2G: East 1 deg",
        0.0, 0.0,
        { 0.0, DEG_LAT_M, 0.0 },
        { 0.0, 1.0, 971.37 }
    );

    // --- Section 2: Geo to NED ---
    std::cout << "\n[Geo -> NED Tests]\n";

    // 1. Identity
    // If Target == Origin, NED should be (0,0,0)
    RunTest_GeoToNed("G2N: Identity",
        10.5, 20.5,             // Origin
        { 10.5, 20.5, 0.0f },   // Target
        { 0.0, 0.0, 0.0 }       // Expected NED
    );

    // 2. North Check
    // Origin: (0,0). Target: (1,0). Expected North: ~111km
    RunTest_GeoToNed("G2N: North 1 deg",
        0.0, 0.0,
        { 1.0, 0.0, 0.0f },
        { DEG_LAT_M, 0.0, 0.0 }
    );

    // 3. East Check
    // Origin: (0,0). Target: (0,1). Expected East: ~111km
    RunTest_GeoToNed("G2N: East 1 deg",
        0.0, 0.0,
        { 0.0, 1.0, 0.0f },
        { 0.0, DEG_LAT_M, 0.0 }
    );

    // 4. Altitude Check (Down)
    // Origin Alt: 0. Target Alt: 100.
    // In NED, "Up" is negative Down. So Down should be -100.
    RunTest_GeoToNed("G2N: Altitude 100m",
        32.0, 34.0,              // Origin
        { 32.0, 34.0, 100.0f },  // Target (100m high)
        { 0.0, 0.0, -100.0 }     // Expected (Down = -100)
    );

    std::cout << "\nSummary: Passed: " << g_tests_passed << ", Failed: " << g_tests_failed << std::endl;
    if (g_logFile.is_open()) g_logFile.close();
    return (g_tests_failed == 0) ? 0 : 1;
#endif
}
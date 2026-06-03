#pragma once

#include "api_structs.h"

#include <array>
#include <cmath>
#include <cstdint>

// --- Constants ---
namespace SphericalConsts
{
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 6.28318530717958647692;
    constexpr double EARTH_RADIUS_M = 6371000.0;
    constexpr double EPS_RAD = 1e-10;       // ~0.6mm on Earth surface
    constexpr double EPS_RAD_BOUNDARY = 1e-8; // ~0.06m tolerance for on-boundary detection
}

// --- Vec3 type (stack-only, fixed-size) ---
using Vec3 = std::array<double, 3>;

// --- Vector Math Helpers ---

inline double Vec3Dot(const Vec3& a, const Vec3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline Vec3 Vec3Cross(const Vec3& a, const Vec3& b)
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    };
}

inline double Vec3Norm(const Vec3& v)
{
    return std::sqrt(Vec3Dot(v, v));
}

inline Vec3 Vec3Normalize(const Vec3& v)
{
    double n = Vec3Norm(v);
    if (n < 1e-18) return { 0.0, 0.0, 0.0 };
    return { v[0] / n, v[1] / n, v[2] / n };
}

// --- Coordinate Conversion ---

inline double DegToRad(double degrees)
{
    return degrees * (SphericalConsts::PI / 180.0);
}

inline double RadToDeg(double radians)
{
    return radians * (180.0 / SphericalConsts::PI);
}

/**
 * @brief Converts geodetic lat/lon (degrees) to a unit vector on the Earth-centered unit sphere.
 */
inline Vec3 LatLonDegToUnitVector(double latDeg, double lonDeg)
{
    const double lat = DegToRad(latDeg);
    const double lon = DegToRad(lonDeg);
    const double clat = std::cos(lat);
    return {
        clat * std::cos(lon),
        clat * std::sin(lon),
        std::sin(lat)
    };
}

// --- Spherical Geometry Functions ---

/**
 * @brief Computes the angular distance (radians) between two unit vectors on the sphere.
 */
double AngularDistanceRad(const Vec3& a, const Vec3& b);

/**
 * @brief Checks if a point lies on the shorter great-circle arc between two endpoints.
 * @param edge_start Unit vector of arc start
 * @param edge_end Unit vector of arc end
 * @param point Unit vector of query point
 * @param eps_rad Tolerance in radians
 * @return true if point is on the arc segment
 */
bool IsPointOnGreatCircleSegment(const Vec3& edge_start, const Vec3& edge_end, const Vec3& point, double eps_rad);

/**
 * @brief Accumulates the signed winding angle contribution of one polygon edge.
 *
 * Projects the edge endpoints onto the tangent plane at query_point and computes
 * the signed turning angle.
 *
 * @param query_point Unit vector of the query point
 * @param edge_start Unit vector of edge start
 * @param edge_end Unit vector of edge end
 * @param eps_rad Tolerance for boundary detection
 * @param[in,out] sum_signed_angles Running sum of signed angles
 * @return true if the query point is detected on the boundary (early exit)
 */
bool AccumulateEdgeAngle(const Vec3& query_point, const Vec3& edge_start, const Vec3& edge_end, double eps_rad, long double& sum_signed_angles);

/**
 * @brief Computes the shortest distance (meters) from a point to a great-circle arc segment.
 *
 * Uses the cross-track distance formula, clamped to the segment endpoints.
 *
 * @param point Unit vector of query point
 * @param arc_start Unit vector of arc start
 * @param arc_end Unit vector of arc end
 * @return Distance in meters
 */
double CrossTrackDistanceMeters(const Vec3& point, const Vec3& arc_start, const Vec3& arc_end);

/**
 * @brief Checks if two great-circle arc segments intersect.
 *
 * Computes the two intersection points of the great circles defined by each arc,
 * then checks if either lies on both arc segments.
 *
 * @param a1 Unit vector start of first arc
 * @param a2 Unit vector end of first arc
 * @param b1 Unit vector start of second arc
 * @param b2 Unit vector end of second arc
 * @param eps_rad Tolerance
 * @return true if the arcs intersect
 */
bool DoSphericalArcsIntersect(const Vec3& a1, const Vec3& a2, const Vec3& b1, const Vec3& b2, double eps_rad);

/**
 * @brief Computes the destination point given a start point, azimuth, and distance.
 *
 * Uses the spherical destination-point formula:
 *   lat2 = asin(sin(lat1)*cos(d/R) + cos(lat1)*sin(d/R)*cos(azimuth))
 *   lon2 = lon1 + atan2(sin(azimuth)*sin(d/R)*cos(lat1), cos(d/R) - sin(lat1)*sin(lat2))
 *
 * @param startLatDeg Starting latitude in degrees
 * @param startLonDeg Starting longitude in degrees
 * @param azimuthDeg Azimuth (bearing) in degrees from North
 * @param distanceMeters Distance along the great circle in meters
 * @param[out] outLatDeg Destination latitude in degrees
 * @param[out] outLonDeg Destination longitude in degrees
 */
void DestinationPointDeg(double startLatDeg, double startLonDeg, double azimuthDeg, double distanceMeters, double& outLatDeg, double& outLonDeg);

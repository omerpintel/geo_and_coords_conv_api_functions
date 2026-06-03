#include "api_functions.h"
#include "test_utils.h"
#include "no_heap.h"
#include "cov_spy.h"

#include <cstddef>   // for nullptr

#if defined(_DEBUG) || !defined(NDEBUG)
bool g_cov_map[(int)ECovFuncID::MAX_FUNCS][MAX_POINTS_PER_FUNC] = { false };

bool* GetCoverageArray(ECovFuncID funcId) {
    return g_cov_map[(int)funcId];
}

void ResetCoverage() {
    for (int i = 0; i < (int)ECovFuncID::MAX_FUNCS; ++i) {
        for (int j = 0; j < MAX_POINTS_PER_FUNC; ++j) {
            g_cov_map[i][j] = false;
        }
    }
}
#endif

// --- Main API Functions ---

const char* GetApiVersionString() {
    return API_FUNCTIONS_VERSION_STRING;
}

void GetApiVersionNumbers(uint16_t* major, uint16_t* minor, uint16_t* patch) {
    if (major != nullptr) {
        *major = API_FUNCTIONS_VERSION_MAJOR;
    }
    if (minor != nullptr) {
        *minor = API_FUNCTIONS_VERSION_MINOR;
    }
    if (patch != nullptr) {
        *patch = API_FUNCTIONS_VERSION_PATCH;
    }
}

void isInsidePolygonNED(const SPointNE* polygon, uint16_t pointCount, const SPointNE testPoint, float radiusMeters, uint8_t* outResult, uint8_t* resultState) {
    #if defined(_DEBUG) || !defined(NDEBUG)
        const ECovFuncID current_func_id = ECovFuncID::IsInside;
    #endif
    
    COV_POINT(0);

    // Guard against null output pointers - write fail-safe defaults to whichever is valid
    if (outResult == nullptr || resultState == nullptr) {
        if (outResult != nullptr)  { *outResult = true; }
        if (resultState != nullptr) { *resultState = EIsInsideResult::IS_INSIDE_OUTPUT_PTR_IS_NULL; }
        return;
    }

    // Default initialization
    *outResult = true;
    *resultState = EIsInsideResult::IS_INSIDE_OK;

    // 1. Validation Logic
    if (polygon == nullptr) {
        COV_POINT(1);
        *resultState = EIsInsideResult::IS_INSIDE_POLYGON_IS_NULL_PTR;
        return;
    }
    if (pointCount < 3) {
        COV_POINT(2);
        *resultState = EIsInsideResult::IS_INSIDE_POLYGON_WITH_LESS_THAN_3_POINTS;
        return;
    }
    if (pointCount > MAX_POLYGON_VERTICES) {
        *resultState = EIsInsideResult::IS_INSIDE_POLYGON_EXCEEDS_MAX_VERTICES;
        return;
    }

    // --- Ray Casting Algorithm ---
    // Cast a ray to the North and count intersections to determine if
    // the center of the circle is inside the geometric shape.
    bool isCenterInside = false;
    for (size_t i = 0, j = pointCount - 1; i < pointCount; j = i++) {
        COV_POINT(3);
        // Check if edge straddles the test point's East line
        if ((polygon[i].east > testPoint.east) != (polygon[j].east > testPoint.east)) {
            COV_POINT(4);

            // for safety of dividing by zero
            float deltaEast = polygon[j].east - polygon[i].east;
            if (std::abs(deltaEast) < EPSILON) {
                continue;
            }
            // Calculate intersection on North axis 
            // y = y_1 + m*(x-x_1)
            double intersectN = polygon[i].north + ((polygon[j].north - polygon[i].north) / deltaEast) * (testPoint.east - polygon[i].east);

            // Toggle state if intersection is strictly to the North of test point
            if (testPoint.north < intersectN) {
                COV_POINT(5);
                isCenterInside = !isCenterInside;
            }
        }
    }

    // If the center is inside, we definitely collide.
    if (isCenterInside)
    {
        COV_POINT(6);
        *outResult =  true;
        return;
    }

    // Check if circle intersect any edges.
    for (size_t i = 0; i < pointCount; ++i) {
        COV_POINT(7);
        double dSq = getDistToSegmentSquared(testPoint, polygon[i], polygon[(i + 1) % pointCount]);

        // If distance is less than radius, the object hits the wall.
        if (dSq < radiusMeters * radiusMeters && !areAlmostEqual(dSq, radiusMeters * radiusMeters)) {
            COV_POINT(8);
            *outResult = true;
            return;
        }

        // Check if point is exactly on the boundary
        if (areAlmostEqual(dSq, 0.0)) {
            COV_POINT(9);
            *outResult = true;
            return;
        }
    }

    // If we are here, Center is OUTSIDE and Distance > Radius. We are safe.
    COV_POINT(10);
    *outResult = false;
}


void doesLineIntersectPolygonNED(const SPointNE* polygon, uint16_t pointCount, const SPointNE testPoint, float azimuthDegrees, float maxLength, uint8_t* outResult, uint8_t* resultState) {
    #if defined(_DEBUG) || !defined(NDEBUG)
        const ECovFuncID current_func_id = ECovFuncID::Intersect;
    #endif
    
    COV_POINT(0);

    // Guard against null output pointers - write fail-safe defaults to whichever is valid
    if (outResult == nullptr || resultState == nullptr) {
        if (outResult != nullptr)  { *outResult = true; }
        if (resultState != nullptr) { *resultState = ELineIntersectResult::LINE_INTERSECT_OUTPUT_PTR_IS_NULL; }
        return;
    }

    *outResult = true;
    *resultState = ELineIntersectResult::LINE_INTERSECT_OK;

    // 1. Validation
    if (polygon == nullptr) {
        COV_POINT(1);
        *resultState = ELineIntersectResult::LINE_INTERSECT_POLYGON_IS_NULL_PTR;
        return;
    }
    if (pointCount < 3) {
        COV_POINT(2);
        *resultState = ELineIntersectResult::LINE_INTERSECT_POLYGON_WITH_LESS_THAN_3_POINTS;
        return;
    }
    if (pointCount > MAX_POLYGON_VERTICES) {
        *resultState = ELineIntersectResult::LINE_INTERSECT_POLYGON_EXCEEDS_MAX_VERTICES;
        return;
    }
    if (maxLength <= 0.0f) {
        COV_POINT(3);
        *resultState = ELineIntersectResult::LINE_INTERSECT_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO;
        return;
    }

    uint8_t tempResult = false;
    uint8_t tempResultState = EIsInsideResult::IS_INSIDE_OK;

    // If the Start Point is inside the polygon, it is an immediate intersection.
    isInsidePolygonNED(polygon, pointCount, testPoint, 0.0, &tempResult, &tempResultState);
    if (tempResult) {
        COV_POINT(4);
        *outResult = true;
        return;
    }

    // Calculate End Point of the Line
    // NED System: Azimuth 0 is North (+X), 90 is East (+Y).
    double thetaRad = azimuthDegrees * (PI / 180.0);
    SPointNE endPoint;
    endPoint.north = testPoint.north + static_cast<float>(maxLength * std::cos(thetaRad));
    endPoint.east = testPoint.east + static_cast<float>(maxLength * std::sin(thetaRad));

    // Check Intersection with all Polygon Edges
    for (size_t i = 0; i < pointCount; ++i) {
        COV_POINT(5);
        SPointNE p1 = polygon[i];
        SPointNE p2 = polygon[(i + 1) % pointCount];

        if (doSegmentsIntersect(testPoint, endPoint, p1, p2)) {
            COV_POINT(6);
            *outResult = true;
            return;
        }
    }

    *outResult = false;
    return;
}


void GeoToNed(const SPointGeo origin, const SPointGeo geoPoint, SPointNED* resNedPoint)
{
    SPointECEF pointEcef = GeoToEcef(geoPoint);
    SPointNED pointNed = EcefToNed(origin.latitudeDeg, origin.longitudeDeg, origin.altitude, pointEcef);

    *resNedPoint = pointNed;
}


void NedToGeo(const SPointGeo origin, const SPointNED nedPoint, SPointGeo* resGeopoint)
{
    SPointECEF pointEcef = NedToEcef(origin.latitudeDeg, origin.longitudeDeg, origin.altitude, nedPoint);
    SPointGeo pointGeo = EcefToGeo(pointEcef);

    *resGeopoint = pointGeo;
}


// ========================================================================
// GEO (Geodetic / LLA) Coordinate System Functions
// ========================================================================

void isInsidePolygonGeo(const SPointGeo* polygon, uint16_t pointCount, const SPointGeo testPoint, float radiusMeters, uint8_t* outResult, uint8_t* resultState) {
    #if defined(_DEBUG) || !defined(NDEBUG)
        const ECovFuncID current_func_id = ECovFuncID::IsInsideGeo;
    #endif

    COV_POINT(0);

    // Guard against null output pointers
    if (outResult == nullptr || resultState == nullptr) {
        if (outResult != nullptr)  { *outResult = true; }
        if (resultState != nullptr) { *resultState = EIsInsideGeoResult::IS_INSIDE_GEO_OUTPUT_PTR_IS_NULL; }
        return;
    }

    // Default: assume collision (fail-safe)
    *outResult = true;
    *resultState = EIsInsideGeoResult::IS_INSIDE_GEO_OK;

    // Validation
    if (polygon == nullptr) {
        COV_POINT(1);
        *resultState = EIsInsideGeoResult::IS_INSIDE_GEO_POLYGON_IS_NULL_PTR;
        return;
    }
    if (pointCount < 3) {
        COV_POINT(2);
        *resultState = EIsInsideGeoResult::IS_INSIDE_GEO_POLYGON_WITH_LESS_THAN_3_POINTS;
        return;
    }
    if (pointCount > MAX_POLYGON_VERTICES) {
        COV_POINT(3);
        *resultState = EIsInsideGeoResult::IS_INSIDE_GEO_POLYGON_EXCEEDS_MAX_VERTICES;
        return;
    }

    // Convert query point to unit vector
    const Vec3 queryVec = LatLonDegToUnitVector(testPoint.latitudeDeg, testPoint.longitudeDeg);

    // Convert first vertex
    const Vec3 firstVertex = LatLonDegToUnitVector(polygon[0].latitudeDeg, polygon[0].longitudeDeg);
    Vec3 previousVertex = firstVertex;

    long double sumSignedAngles = 0.0L;
    const double eps_rad = SphericalConsts::EPS_RAD_BOUNDARY;

    // --- Spherical Winding Number Algorithm ---
    // Process each edge and accumulate signed angle
    for (size_t i = 1; i < pointCount; ++i) {
        COV_POINT(4);
        const Vec3 currentVertex = LatLonDegToUnitVector(polygon[i].latitudeDeg, polygon[i].longitudeDeg);

        if (AccumulateEdgeAngle(queryVec, previousVertex, currentVertex, eps_rad, sumSignedAngles)) {
            COV_POINT(5);
            // Point is on the boundary — treat as inside
            *outResult = true;
            return;
        }

        previousVertex = currentVertex;
    }

    // Close the polygon: last vertex -> first vertex
    if (AccumulateEdgeAngle(queryVec, previousVertex, firstVertex, eps_rad, sumSignedAngles)) {
        COV_POINT(6);
        *outResult = true;
        return;
    }

    // Check winding number: if |sum| ≈ 2π, point is inside
    constexpr long double kTwoPi = 6.283185307179586476925286766559005768L;
    bool isCenterInside = std::abs(std::abs(sumSignedAngles) - kTwoPi) <= 1e-7L;

    if (isCenterInside) {
        COV_POINT(7);
        *outResult = true;
        return;
    }

    // --- Radius Check ---
    // If center is outside, check if the circle boundary intersects any edge
    if (radiusMeters > 0.0f) {
        COV_POINT(8);
        Vec3 prevVtx = firstVertex;
        for (size_t i = 1; i < pointCount; ++i) {
            const Vec3 currVtx = LatLonDegToUnitVector(polygon[i].latitudeDeg, polygon[i].longitudeDeg);
            double dist = CrossTrackDistanceMeters(queryVec, prevVtx, currVtx);
            if (dist < static_cast<double>(radiusMeters)) {
                COV_POINT(9);
                *outResult = true;
                return;
            }
            prevVtx = currVtx;
        }
        // Closing edge
        double dist = CrossTrackDistanceMeters(queryVec, prevVtx, firstVertex);
        if (dist < static_cast<double>(radiusMeters)) {
            COV_POINT(9);
            *outResult = true;
            return;
        }
    }

    // Center is outside and distance > radius. Safe.
    COV_POINT(10);
    *outResult = false;
}


void doesLineIntersectPolygonGeo(const SPointGeo* polygon, uint16_t pointCount, const SPointGeo testPoint, float azimuthDegrees, float maxLengthMeters, uint8_t* outResult, uint8_t* resultState) {
    #if defined(_DEBUG) || !defined(NDEBUG)
        const ECovFuncID current_func_id = ECovFuncID::IntersectGeo;
    #endif

    COV_POINT(0);

    // Guard against null output pointers
    if (outResult == nullptr || resultState == nullptr) {
        if (outResult != nullptr)  { *outResult = true; }
        if (resultState != nullptr) { *resultState = ELineIntersectGeoResult::LINE_INTERSECT_GEO_OUTPUT_PTR_IS_NULL; }
        return;
    }

    *outResult = true;
    *resultState = ELineIntersectGeoResult::LINE_INTERSECT_GEO_OK;

    // Validation
    if (polygon == nullptr) {
        COV_POINT(1);
        *resultState = ELineIntersectGeoResult::LINE_INTERSECT_GEO_POLYGON_IS_NULL_PTR;
        return;
    }
    if (pointCount < 3) {
        COV_POINT(2);
        *resultState = ELineIntersectGeoResult::LINE_INTERSECT_GEO_POLYGON_WITH_LESS_THAN_3_POINTS;
        return;
    }
    if (pointCount > MAX_POLYGON_VERTICES) {
        COV_POINT(3);
        *resultState = ELineIntersectGeoResult::LINE_INTERSECT_GEO_POLYGON_EXCEEDS_MAX_VERTICES;
        return;
    }
    if (maxLengthMeters <= 0.0f) {
        COV_POINT(4);
        *resultState = ELineIntersectGeoResult::LINE_INTERSECT_GEO_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO;
        return;
    }

    // Check if start point is inside polygon
    uint8_t tempResult = false;
    uint8_t tempState = EIsInsideGeoResult::IS_INSIDE_GEO_OK;
    isInsidePolygonGeo(polygon, pointCount, testPoint, 0.0f, &tempResult, &tempState);
    if (tempResult) {
        COV_POINT(5);
        *outResult = true;
        return;
    }

    // Compute line endpoint using destination-point formula
    double endLatDeg = 0.0, endLonDeg = 0.0;
    DestinationPointDeg(testPoint.latitudeDeg, testPoint.longitudeDeg, azimuthDegrees, maxLengthMeters, endLatDeg, endLonDeg);

    const Vec3 lineStart = LatLonDegToUnitVector(testPoint.latitudeDeg, testPoint.longitudeDeg);
    const Vec3 lineEnd = LatLonDegToUnitVector(endLatDeg, endLonDeg);

    // Check intersection of line arc with each polygon edge
    const Vec3 firstVertex = LatLonDegToUnitVector(polygon[0].latitudeDeg, polygon[0].longitudeDeg);
    Vec3 previousVertex = firstVertex;

    const double eps_rad = SphericalConsts::EPS_RAD_BOUNDARY;

    for (size_t i = 1; i < pointCount; ++i) {
        COV_POINT(6);
        const Vec3 currentVertex = LatLonDegToUnitVector(polygon[i].latitudeDeg, polygon[i].longitudeDeg);

        if (DoSphericalArcsIntersect(lineStart, lineEnd, previousVertex, currentVertex, eps_rad)) {
            COV_POINT(7);
            *outResult = true;
            return;
        }

        previousVertex = currentVertex;
    }

    // Closing edge: last vertex -> first vertex
    if (DoSphericalArcsIntersect(lineStart, lineEnd, previousVertex, firstVertex, eps_rad)) {
        COV_POINT(7);
        *outResult = true;
        return;
    }

    COV_POINT(8);
    *outResult = false;
}

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

void isInsidePolygon(const Point* polygon, uint16_t pointCount, const Point* testPoint, float radiusMeters, bool* outResult, EResultState* resultState) {
    #if defined(_DEBUG) || !defined(NDEBUG)
        const ECovFuncID current_func_id = ECovFuncID::IsInside;
    #endif
    
    COV_POINT(0);

    // Default initialization
    *outResult = true;
    *resultState = EResultState::OK;

    // 1. Validation Logic
    if (polygon == nullptr) {
        COV_POINT(1);
        *resultState = EResultState::POLYGON_IS_NULL_PTR;
        return;
    }
    if (pointCount < 3) {
        COV_POINT(2);
        *resultState = EResultState::POLYGON_WITH_LESS_THAN_3_POINTS;
        return;
    }

    // --- Ray Casting Algorithm ---
    // Cast a ray to the North and count intersections to determine if
    // the center of the circle is inside the geometric shape.
    bool isCenterInside = false;
    for (size_t i = 0, j = pointCount - 1; i < pointCount; j = i++) {
        COV_POINT(3);
        // Check if edge straddles the test point's East line
        if ((polygon[i].east > testPoint->east) != (polygon[j].east > testPoint->east)) {
            COV_POINT(4);

            // for safety of dividing by zero
            float deltaEast = polygon[j].east - polygon[i].east;
            if (std::abs(deltaEast) < EPSILON) {
                continue;
            }
            // Calculate intersection on North axis 
            // y = y_1 + m*(x-x_1)
            double intersectN = polygon[i].north + ((polygon[j].north - polygon[i].north) / deltaEast) * (testPoint->east - polygon[i].east);

            // Toggle state if intersection is strictly to the North of test point
            if (testPoint->north < intersectN) {
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
        double dSq = getDistToSegmentSquared(*testPoint, polygon[i], polygon[(i + 1) % pointCount]);

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


void doesLineIntersectPolygon(const Point* polygon, uint16_t pointCount, const Point* testPoint, float azimuthDegrees, float maxLength, bool* outResult, EResultState* resultState) {
    #if defined(_DEBUG) || !defined(NDEBUG)
        const ECovFuncID current_func_id = ECovFuncID::Intersect;
    #endif
    
    COV_POINT(0);

    *outResult = true;
    *resultState = EResultState::OK;

    // 1. Validation
    if (polygon == nullptr) {
        COV_POINT(1);
        *resultState = EResultState::POLYGON_IS_NULL_PTR;
        return;
    }
    if (pointCount < 3) {
        COV_POINT(2);
        *resultState = EResultState::POLYGON_WITH_LESS_THAN_3_POINTS;
        return;
    }
    if (maxLength <= 0.0f) {
        COV_POINT(3);
        *resultState = EResultState::MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO;
        return;
    }

    bool tempResult = false;
    EResultState tempResultState = EResultState::OK;

    // If the Start Point is inside the polygon, it is an immediate intersection.
    isInsidePolygon(polygon, pointCount, testPoint, 0.0, &tempResult, &tempResultState);
    if (tempResult) {
        COV_POINT(4);
        *outResult = true;
        return;
    }

    // Calculate End Point of the Line
    // NED System: Azimuth 0 is North (+X), 90 is East (+Y).
    double thetaRad = azimuthDegrees * (PI / 180.0);
    Point endPoint;
    endPoint.north = testPoint->north + maxLength * std::cos(thetaRad);
    endPoint.east = testPoint->east + maxLength * std::sin(thetaRad);

    // Check Intersection with all Polygon Edges
    for (size_t i = 0; i < pointCount; ++i) {
        COV_POINT(5);
        Point p1 = polygon[i];
        Point p2 = polygon[(i + 1) % pointCount];

        if (doSegmentsIntersect(*testPoint, endPoint, p1, p2)) {
            COV_POINT(6);
            *outResult = true;
            return;
        }
    }

    *outResult = false;
    return;
}


SPointNED GeoToNed(const double* originLatitudeDeg, const double* originLongitudeDeg, const SPointGeo* geoPoint)
{
    double latitudeRad = *originLatitudeDeg * PI / 180.0;
    double longitudeRad = *originLongitudeDeg * PI / 180.0;

    SPointECEF pointEcef = GeoToEcef(geoPoint);
    SPointNED pointNed = EcefToNed(&latitudeRad, &latitudeRad, &pointEcef);

    return pointNed;
}


SPointGeo NedToGeo(const double* originLatitudeDeg, const double* originLongitudeDeg, const SPointNED* nedPoint)
{
    double latitudeRad = *originLatitudeDeg *  PI / 180.0;;
    double longitudeRad = *originLongitudeDeg * PI / 180.0;

    SPointECEF pointEcef = NedToEcef(&latitudeRad, &longitudeRad, nedPoint);
    SPointGeo pointGeo = EcefToGeo(&pointEcef);

    return pointGeo;
}
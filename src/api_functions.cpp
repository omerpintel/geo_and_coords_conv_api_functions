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

// --- Helper Functions ---
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


// Checks if two double values are effectively equal.
// Uses a scaled epsilon comparison to handle floating-point precision errors.
static bool areAlmostEqual(const float a, const float b) {
    return std::fabs(a - b) <= EPSILON * 100.0f;
}


// Calculates the squared Euclidean distance between two points.
// Using squared distance avoids expensive square root operations during comparisons.
static double getDistSq(const Point& a, const Point& b) {
    double dn = a.north - b.north;
    double de = a.east - b.east;
    return dn * dn + de * de;
}


// Calculates the squared shortest distance from a point to a line segment.
static double getDistToSegmentSquared(const Point& p, const Point& a, const Point& b) {
    const float l2 = getDistSq(a, b);

    // If start and end points are identical, return distance to point 'a'
    if (l2 == 0.0) return getDistSq(p, a);

    // Calculate projection factor t represents the relative position of the projection on the infinite line:
    // 0.0 = Start (a), 1.0 = End (b).
    // t = [(p-a) . (b-a)] / |b-a|^2
    float t = ((p.north - a.north) * (b.north - a.north) +
        (p.east - a.east) * (b.east - a.east)) / l2;

    // Clamp t to the segment [0, 1] to handle points beyond endpoints
    if (t < 0.0) t = 0.0;
    else if (t > 1.0) t = 1.0;

    Point projection = {
        a.north + t * (b.north - a.north),
        a.east + t * (b.east - a.east)
    };

    return getDistSq(p, projection);
}


// --- Intersection Helper Functions ---

// Checks if point q lies on the line segment pr.
// Assumes points are already known to be collinear.
static bool onSegment(const Point& p, const Point& q, const Point& r) {
    return q.north <= MAX(p.north, r.north) && q.north >= MIN(p.north, r.north) &&
        q.east <= MAX(p.east, r.east) && q.east >= MIN(p.east, r.east);
}


// Determines the orientation of the ordered triplet (p, q, r).
 // return 0 if collinear, 1 if clockwise, 2 if counter-clockwise.
static int orientation(const Point& p, const Point& q, const Point& r) {
    double val = (q.east - p.east) * (r.north - q.north) -
        (q.north - p.north) * (r.east - q.east);

    if (areAlmostEqual(val, 0.0)) return 0;
    return (val > 0) ? 1 : 2;
}


// Checks if two line segments (p1-q1 and p2-q2) intersect.
// Uses the general case and special cases (collinear points) of the orientation method.
static bool doSegmentsIntersect(const Point& p1, const Point& q1, const Point& p2, const Point& q2) {
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    // General Case: Segments straddle each other
    if (o1 != o2 && o3 != o4) return true;

    // Special Cases: Collinear points lying on segments
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;

    return false;
}

// --- Coords Helper Functions ---
static double NavValidateLatitude(const double& inLatitude)
{
    double latitude;

    if (inLatitude > (PI / 2)) { 
        latitude = PI - inLatitude; 
    }
    else
    {
        if (inLatitude < -(PI / 2)) {
            latitude = -PI - inLatitude;
        }
        else {
            latitude = inLatitude;
        }
    }
    return latitude;
}

static double NavValidateLongitude(const double& inLongitude)
{
    double longitude;

    if (inLongitude > PI) {
        longitude = inLongitude - (2 * PI);
    }
    else
    {
        if (inLongitude < -PI) {
            longitude = inLongitude + (2 * PI);
        }
        else {
            longitude = inLongitude;
        }
    }
    return longitude;
}

static void MulMatVec3(const double A[3][3], const double VIn[3], double VOut[3])
{
    VOut[0] = A[0][0] * VIn[0] + A[0][1] * VIn[1] + A[0][2] * VIn[2];
    VOut[1] = A[1][0] * VIn[0] + A[1][1] * VIn[1] + A[1][2] * VIn[2];
    VOut[2] = A[2][0] * VIn[0] + A[2][1] * VIn[1] + A[2][2] * VIn[2];
}

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


SPointECEF GeoToEcef(const SPointGeo* geoPoint)
{
    double latitude, longitude, altitude;
    double rn, rn_plus_h_cos_lat;

    const double oneMinusE2 = 1 - WGS84::E2;

    // Latitude is valid in [-pi/2, pi/2]
    latitude = geoPoint->latitude;

    // Longitude is valid in [-pi, pi]
    longitude = geoPoint->longitude;

    altitude = geoPoint->altitude;

    // Normal (east/west) prime vertical curvature radii (m)
    rn = WGS84::RN(latitude);

    rn_plus_h_cos_lat = ((rn + altitude) * std::cos(latitude));

    // ECEF (m) position coordinates
    SPointECEF ecef;
    ecef.x = rn_plus_h_cos_lat * std::cos(longitude);
    ecef.y = rn_plus_h_cos_lat * std::sin(longitude);
    ecef.z = (oneMinusE2 * rn + altitude) * std::sin(latitude);

    return ecef;
}


SPointGeo EcefToGeo(const SPointECEF* ecefPoint)
{
    const double oneMinusE2 = 1 - WGS84::E2;
    const double sqrtOneMinusE2 = std::sqrt(oneMinusE2);
    const double invOneMinusE2 = 1 / oneMinusE2;

    double x, y, z;
    x = ecefPoint->x;
    y = ecefPoint->y;
    z = ecefPoint->z;

    SPointGeo geo;
    geo.longitude = std::atan2(y, x);

    double normXPosYPos = API_UTILS::safe_sqrt((y * y + x * x), 1.0);
    double inv1 = API_UTILS::safe_div(1.0, (normXPosYPos * sqrtOneMinusE2), 1.0);
    double u = std::atan(z * inv1);
    double tmp2 = z + WGS84::E2 * invOneMinusE2 * EARTH_CONSTS::R0 * sqrtOneMinusE2 * std::sin(u) * std::sin(u) * std::sin(u);
    double den = normXPosYPos - WGS84::E2 * EARTH_CONSTS::R0 * std::cos(u) * std::cos(u) * std::cos(u);
    double invDen = API_UTILS::safe_div(1.0, den, 1.0);
    geo.latitude = std::atan(tmp2 * invDen);
    double sinlat = std::sin(geo.latitude);
    double sinlat2 = sinlat * sinlat;
    double coslat = std::cos(geo.latitude);
    double inv2;

    if (sinlat2 <= 0.5)
    {
        inv2 = API_UTILS::safe_div(1.0, (API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) * coslat), 1.0);
        geo.altitude = (normXPosYPos * API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) - EARTH_CONSTS::R0 * coslat) * inv2;
    }
    else
    {
        inv2 = API_UTILS::safe_div(1.0, (API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) * sinlat), 1.0);
        geo.altitude = (z * API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) - EARTH_CONSTS::R0 * oneMinusE2 * sinlat) * inv2;
    }

    return geo;
}


SPointNED EcefToNed(const double* latitude, const double* longitude, const SPointECEF* ecefPoint)
{
    double localLatitude = NavValidateLatitude(*latitude);
    double localLongitude = NavValidateLongitude(*longitude);
    double sinLat = std::sin(localLatitude);
    double cosLat = std::cos(localLatitude);
    double sinLong = std::sin(localLongitude);
    double cosLong = std::cos(localLongitude);

    double tempMat[3][3];
    tempMat[0][0] = -sinLat * cosLong;
    tempMat[0][1] = -sinLat * sinLong;
    tempMat[0][2] = cosLat;
    tempMat[1][0] = -sinLong;
    tempMat[1][1] = cosLong;
    tempMat[1][2] = 0.0;
    tempMat[2][0] = -cosLat * cosLong;
    tempMat[2][1] = -cosLat * sinLong;
    tempMat[2][2] = -sinLat;

    double ecefVec[3] = { ecefPoint->x,ecefPoint->y,ecefPoint->z };
    double nedVec[3];
    MulMatVec3(tempMat, ecefVec, nedVec);

    SPointNED ned;
    ned.north = nedVec[0];
    ned.east = nedVec[1];
    ned.down = nedVec[2];

    return ned;
}
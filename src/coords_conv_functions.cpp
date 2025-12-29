#include "coords_conv_functions.h"

// --- helper functions ---

double NavValidateLatitude(const double& inLatitude)
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

double NavValidateLongitude(const double& inLongitude)
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

void MulMatVec3(const double A[3][3], const double VIn[3], double VOut[3])
{
    VOut[0] = A[0][0] * VIn[0] + A[0][1] * VIn[1] + A[0][2] * VIn[2];
    VOut[1] = A[1][0] * VIn[0] + A[1][1] * VIn[1] + A[1][2] * VIn[2];
    VOut[2] = A[2][0] * VIn[0] + A[2][1] * VIn[1] + A[2][2] * VIn[2];
}

// --- main functions ---

SPointECEF GeoToEcef(const SPointGeo* geoPoint)
{
    double latitudeRad, longitudeRad, altitude;
    double rn, rn_plus_h_cos_lat;

    const double oneMinusE2 = 1 - WGS84::E2;

    // Latitude is valid in [-pi/2, pi/2]
    latitudeRad = geoPoint->latitudeDeg * PI / 180.0;

    // Longitude is valid in [-pi, pi]
    longitudeRad = geoPoint->longitudeDeg * PI / 180.0;

    altitude = geoPoint->altitude;

    // Normal (east/west) prime vertical curvature radii (m)
    rn = WGS84::RN(latitudeRad);

    rn_plus_h_cos_lat = (rn + altitude) * std::cos(latitudeRad);

    // ECEF (m) position coordinates
    SPointECEF ecef;
    ecef.x = rn_plus_h_cos_lat * std::cos(longitudeRad);
    ecef.y = rn_plus_h_cos_lat * std::sin(longitudeRad);
    ecef.z = (oneMinusE2 * rn + altitude) * std::sin(latitudeRad);

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

    double longitudeRad, latitudeRad, altitude;
    longitudeRad = std::atan2(y, x);

    double normXPosYPos = API_UTILS::safe_sqrt((y * y + x * x), 1.0);
    double inv1 = API_UTILS::safe_div(1.0, (normXPosYPos * sqrtOneMinusE2), 1.0);
    double u = std::atan(z * inv1);
    double tmp2 = z + WGS84::E2 * invOneMinusE2 * EARTH_CONSTS::R0 * sqrtOneMinusE2 * std::sin(u) * std::sin(u) * std::sin(u);
    double den = normXPosYPos - WGS84::E2 * EARTH_CONSTS::R0 * std::cos(u) * std::cos(u) * std::cos(u);
    double invDen = API_UTILS::safe_div(1.0, den, 1.0);
    latitudeRad = std::atan(tmp2 * invDen);
    double sinlat = std::sin(latitudeRad);
    double sinlat2 = sinlat * sinlat;
    double coslat = std::cos(latitudeRad);
    double inv2;

    if (sinlat2 <= 0.5)
    {
        inv2 = API_UTILS::safe_div(1.0, (API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) * coslat), 1.0);
        altitude = (normXPosYPos * API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) - EARTH_CONSTS::R0 * coslat) * inv2;
    }
    else
    {
        inv2 = API_UTILS::safe_div(1.0, (API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) * sinlat), 1.0);
        altitude = (z * API_UTILS::safe_sqrt((1.0 - WGS84::E2 * sinlat2), 1.0) - EARTH_CONSTS::R0 * oneMinusE2 * sinlat) * inv2;
    }

    SPointGeo geo;
    geo.latitudeDeg = latitudeRad * 180.0 / PI;
    geo.longitudeDeg = longitudeRad * 180.0 / PI;
    geo.altitude = altitude;

    return geo;
}


SPointNED EcefToNed(const double* latitudeRad, const double* longitudeRad, const SPointECEF* ecefPoint)
{
    double localLatitude = NavValidateLatitude(*latitudeRad);
    double localLongitude = NavValidateLongitude(*longitudeRad);
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


SPointECEF NedToEcef(const double* latitudeRad, const double* longitudeRad, const SPointNED* nedPoint)
{
    double localLatitude = NavValidateLatitude(*latitudeRad);
    double localLongitude = NavValidateLongitude(*longitudeRad);
    double sinLat = std::sin(localLatitude);
    double cosLat = std::cos(localLatitude);
    double sinLong = std::sin(localLongitude);
    double cosLong = std::cos(localLongitude);

    double tempMat[3][3];
    tempMat[0][0] = -sinLat * cosLong;
    tempMat[1][0] = -sinLat * sinLong;
    tempMat[2][0] = cosLat;
    tempMat[0][1] = -sinLong;
    tempMat[1][1] = cosLong;
    tempMat[2][1] = 0.0;
    tempMat[0][2] = -cosLat * cosLong;
    tempMat[1][2] = -cosLat * sinLong;
    tempMat[2][2] = -sinLat;

    double nedVec[3] = { nedPoint->north, nedPoint->east, nedPoint->down };
    double ecefVec[3];
    MulMatVec3(tempMat, nedVec, ecefVec);

    double RN = WGS84::RN(localLatitude);
    SPointECEF ecef;

    // ------- CHECK !! DEFFERENT FROM ORIGINAL ALGO --------
    ecef.x = RN * cosLat * cosLong + ecefVec[0];
    ecef.y = RN * cosLat * sinLong + ecefVec[1];
    ecef.z = RN * (1 - WGS84::E2) * sinLat + ecefVec[2];
    //ecef.x = ecefVec[0];
    //ecef.y = ecefVec[1];
    //ecef.z = ecefVec[2];

    return ecef;
}

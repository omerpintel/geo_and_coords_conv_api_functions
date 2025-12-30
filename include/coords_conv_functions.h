#pragma once

#include "api_structs.h"
#include "api_utils.h"

// --- helper functions ---

double NavValidateLatitude(const double& inLatitude);

double NavValidateLongitude(const double& inLongitude);

void MulMatVec3(const double A[3][3], const double VIn[3], double VOut[3]);

// --- main functions ---

SPointECEF GeoToEcef(const SPointGeo* geoPoint);

SPointGeo EcefToGeo(const SPointECEF* ecefPoint);

SPointNED EcefToNed(const double* latitudeRad, const double* longitudeRad, const double* altitude, const SPointECEF* ecefPoint);

SPointECEF NedToEcef(const double* latitudeRad, const double* longitudeRad, const double* altitude, const SPointNED* nedPoint);

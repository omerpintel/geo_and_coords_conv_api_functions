#pragma once

#if defined(_WIN32)
#if defined(POLYGON_LIB_EXPORTS)
#define API_FUNCTIONS __declspec(dllexport)
#else
#define API_FUNCTIONS __declspec(dllimport)
#endif
#else
#define API_FUNCTIONS
#endif

#include "api_structs.h"
#include "api_utils.h"

// --- helper functions ---

double NavValidateLatitude(const double& inLatitude);

double NavValidateLongitude(const double& inLongitude);

void MulMatVec3(const double A[3][3], const double VIn[3], double VOut[3]);

// --- main functions ---

extern "C" {
API_FUNCTIONS SPointECEF GeoToEcef(const SPointGeo* geoPoint);

API_FUNCTIONS SPointGeo EcefToGeo(const SPointECEF* ecefPoint);

API_FUNCTIONS SPointNED EcefToNed(const double* latitude, const double* longitude, const SPointECEF* ecefPoint);

}
#pragma once

#include <cstdint>
#include <cstddef> 
#include <cmath>     // for fabs,sin,cos

// --- Constants ---
const double PI = 3.14159265358979323846;
const double EPSILON = 1e-9;

namespace WGS84
{
	constexpr double F = 3.352810664747481e-003;           // Flattening
	constexpr double A = 6.378137e6;                     // Semi-major axis (meters)
	constexpr double E2 = 2.0 * F - F * F;              // Eccentricity squared
	inline double W2(double latitude) { return 1 - E2 * std::sin(latitude) * std::sin(latitude); } // RT_OMEGA_WGS^2
	inline double W(double latitude) { return std::sqrt(W2(latitude)); } // RT_OMEGA_WGS
	inline double RN(double latitude) { return A / W(latitude); } // Normal (east/west) prime vertical curvature radii (m)
}

namespace EARTH_CONSTS
{
	constexpr double R0 = 6378137.0; // Nominal radius of earth (m)
}

namespace API_UTILS
{
	inline double safe_sqrt(double x, double default_value = 0.0) { return (x >= 0.0) ? std::sqrt(x) : default_value; }
	inline double safe_div(double x, double y, double default_value = 0.0) { return (std::abs(y) > EPSILON) ? (x / y) : default_value; }
}
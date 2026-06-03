#pragma once

#if defined(_WIN32)
	#if defined(API_FUNCTIONS_LIB_EXPORTS)
		#define API_FUNCTIONS __declspec(dllexport)
	#else
		#define API_FUNCTIONS __declspec(dllimport)
	#endif
#else
	#define API_FUNCTIONS
#endif

#include "api_structs.h"
#include "api_version.h"
#include "geometric_functions.h"
#include "coords_conv_functions.h"
#include "spherical_functions.h"

extern "C" {

	// ========================================================================
	// Version Functions
	// ========================================================================

	/**
	 * @brief Returns the semantic version string for this API library.
	 *
	 * The returned pointer refers to a static string literal owned by the
	 * library and must not be modified or freed by the caller.
	 *
	 * @return Version string in MAJOR.MINOR.PATCH format.
	 */
	API_FUNCTIONS const char* GetApiVersionString();

	/**
	 * @brief Returns the semantic version numbers for this API library.
	 *
	 * Each output pointer is optional. Null pointers are ignored so callers may
	 * request only the fields they need.
	 *
	 * @param[out] major Major version, or null.
	 * @param[out] minor Minor version, or null.
	 * @param[out] patch Patch version, or null.
	 */
	API_FUNCTIONS void GetApiVersionNumbers(
		uint16_t* major,
		uint16_t* minor,
		uint16_t* patch
	);

	// ========================================================================
	// NED Coordinate System Functions
	// ========================================================================

	/**
	 * @brief Determines if a circular area is contained within a polygon (NED coordinates).
	 *
	 * This function checks if a test point (representing the center of a circle)
	 * lies inside a polygon and if the circle of a given radius is fully clear
	 * out of the polygon's boundaries.
	 *
	 * @param[in] polygon      Pointer to an array of Point structures defining the polygon vertices (NED meters).
	 * @param[in] pointCount   The number of vertices in the polygon array (max MAX_POLYGON_VERTICES).
	 * @param[in] testPoint    The center point of the test circle (in NED meters).
	 * @param[in] radiusMeters The radius of the object/circle in meters.
	 * - If radius > 0: Checks if the entire circle is strictly outside.
	 * - If radius == 0: Checks if the point itself is outside.
	 * @param[out] outResult   1 = collision/inside, 0 = safe/outside.
	 * @param[out] resultState EIsInsideResult error code.
	 */
	API_FUNCTIONS void isInsidePolygonNED(
		const SPointNE* polygon,
		uint16_t pointCount,
		const SPointNE testPoint,
		float radiusMeters,
		uint8_t* outResult,
		uint8_t* resultState
	);

	/**
	 * @brief Checks if a defined line segment intersects with a polygon (NED coordinates).
	 *
	 * This function determines if a line, defined by a start point, azimuth, and length,
	 * interacts with the polygon geometry. An intersection is detected if:
	 * 1. The line segment crosses any edge of the polygon.
	 * 2. The line segment touches a vertex or lies on an edge.
	 * 3. The starting point of the line is already inside the polygon.
	 *
	 * @param[in] polygon         Pointer to an array of Point structures defining the polygon vertices (NED meters).
	 * @param[in] pointCount      The number of vertices in the polygon array (max MAX_POLYGON_VERTICES).
	 * @param[in] testPoint       The starting point of the line segment (in NED meters).
	 * @param[in] azimuthDegrees  The direction of the line in degrees relative to North (0 = North, 90 = East).
	 * @param[in] maxLengthMeters The length of the line segment in meters.
	 * @param[out] outResult      1 = intersection detected, 0 = no intersection.
	 * @param[out] resultState    ELineIntersectResult error code.
	 */
	API_FUNCTIONS void doesLineIntersectPolygonNED(
		const SPointNE* polygon,
		uint16_t pointCount,
		const SPointNE testPoint,
		float azimuthDegrees,
		float maxLengthMeters,
		uint8_t* outResult,
		uint8_t* resultState
	);

	// ========================================================================
	// GEO (Geodetic / LLA) Coordinate System Functions
	// ========================================================================

	/**
	 * @brief Determines if a circular area is contained within a polygon (Geodetic coordinates).
	 *
	 * Uses full spherical geometry (winding number algorithm on the unit sphere).
	 * Polygon edges are interpreted as the shorter great-circle arc between consecutive vertices.
	 * Radius check uses great-circle cross-track distance.
	 *
	 * Assumptions:
	 * - Polygon is simple and non-self-intersecting.
	 * - No adjacent duplicate or antipodal vertices.
	 * - Altitude is ignored (2D surface geometry).
	 *
	 * @param[in] polygon      Pointer to an array of SPointGeo structures (lat/lon in degrees).
	 * @param[in] pointCount   The number of vertices (>= 3, <= MAX_POLYGON_VERTICES).
	 * @param[in] testPoint    The center point of the test circle (lat/lon in degrees, altitude ignored).
	 * @param[in] radiusMeters The radius of the object/circle in meters.
	 * - If radius > 0: Checks if the circle boundary intersects any polygon edge.
	 * - If radius == 0: Checks if the point itself is inside.
	 * @param[out] outResult   1 = collision/inside, 0 = safe/outside.
	 * @param[out] resultState EIsInsideGeoResult error code.
	 */
	API_FUNCTIONS void isInsidePolygonGeo(
		const SPointGeo* polygon,
		uint16_t pointCount,
		const SPointGeo testPoint,
		float radiusMeters,
		uint8_t* outResult,
		uint8_t* resultState
	);

	/**
	 * @brief Checks if a defined line segment intersects with a polygon (Geodetic coordinates).
	 *
	 * Uses full spherical geometry. The line is a great-circle arc from the start point
	 * along the given azimuth for the specified distance. Intersection is detected if:
	 * 1. The line arc crosses any polygon edge (great-circle arc intersection).
	 * 2. The starting point is already inside the polygon.
	 *
	 * Assumptions:
	 * - Polygon is simple and non-self-intersecting.
	 * - No adjacent duplicate or antipodal vertices.
	 * - Altitude is ignored (2D surface geometry).
	 *
	 * @param[in] polygon         Pointer to an array of SPointGeo structures (lat/lon in degrees).
	 * @param[in] pointCount      The number of vertices (>= 3, <= MAX_POLYGON_VERTICES).
	 * @param[in] testPoint       The starting point of the line (lat/lon in degrees).
	 * @param[in] azimuthDegrees  The direction of the line in degrees from North (0 = North, 90 = East).
	 * @param[in] maxLengthMeters The length of the line segment in meters (along the great circle).
	 * @param[out] outResult      1 = intersection detected, 0 = no intersection.
	 * @param[out] resultState    ELineIntersectGeoResult error code.
	 */
	API_FUNCTIONS void doesLineIntersectPolygonGeo(
		const SPointGeo* polygon,
		uint16_t pointCount,
		const SPointGeo testPoint,
		float azimuthDegrees,
		float maxLengthMeters,
		uint8_t* outResult,
		uint8_t* resultState
	);

	// ========================================================================
	// Coordinate Transformation Functions
	// ========================================================================

	API_FUNCTIONS void GeoToNed(
		const SPointGeo origin,
		const SPointGeo geoPoint,
		SPointNED* resNedPoint
	);

	API_FUNCTIONS void NedToGeo(
		const SPointGeo origin,
		const SPointNED nedPoint,
		SPointGeo* resGeoPoint
	);
}

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

#include "api_utils.h"
#include "api_structs.h"
#include "geometric_functions.h"
#include "coords_conv_functions.h"

extern "C" {
	/**
	 * @brief Determines if a circular area is contained within a polygon.
	 *
	 * This function checks if a test point (representing the center of a circle)
	 * lies inside a polygon and if the circle of a given radius is fully clear
	 * out of the polygon's boundaries.
	 *
	 * @param[in] polygon      Pointer to an array of Point structures defining the polygon vertices.
	 * @param[in] pointCount   The number of vertices in the polygon array.
	 * @param[in] testPoint    The center point of the test circle (in NED meters).
	 * @param[in] origin       Reference point for the coordinate system (Unused; kept for API consistency).
	 * @param[in] radiusMeters The radius of the object/circle in meters.
	 * - If radius > 0: Checks if the entire circle is strictly outside.
	 * - If radius == 0: Checks if the point itself is outside.
	 *
	 * @return true if the circle/point is inside the polygon boundary; false otherwise.
	 */
	API_FUNCTIONS void isInsidePolygon(
		const Point* polygon,
		uint16_t pointCount,
		const Point* testPoint,
		float radiusMeters,
		bool* outResult,
		EResultState* resultState
	);

	/**
	 * @brief Checks if a defined line segment intersects with a polygon.
	 *
	 * This function determines if a line, defined by a start point, azimuth, and length,
	 * interacts with the polygon geometry. An intersection is detected if:
	 * 1. The line segment crosses any edge of the polygon.
	 * 2. The line segment touches a vertex or lies on an edge.
	 * 3. The starting point of the line is already inside the polygon.
	 *
	 * @param[in] polygon         Pointer to an array of Point structures defining the polygon vertices.
	 * @param[in] pointCount      The number of vertices in the polygon array.
	 * @param[in] testPoint       The starting point of the line segment (in NED meters).
	 * @param[in] origin          Reference point for the coordinate system (Unused; kept for API consistency).
	 * @param[in] azimuthDegrees  The direction of the line in degrees relative to North (0° = North, 90° = East).
	 * @param[in] maxLengthMeters The length of the line segment in meters.
	 *
	 * @return true if the line intersects the polygon or originates inside it; false otherwise.
	 */
	API_FUNCTIONS void doesLineIntersectPolygon(
		const Point* polygon,
		uint16_t pointCount,
		const Point* testPoint,
		float azimuthDegrees,
		float maxLengthMeters,
		bool* outResult,
		EResultState* resultState
	);

	API_FUNCTIONS SPointNED GeoToNed(
		const double* originLatitudeDeg,
		const double* originLongitudeDeg,
		const double* originAltitude,
		const SPointGeo* geoPoint
	);

	API_FUNCTIONS SPointGeo NedToGeo(
		const double* originLatitudeDeg, 
		const double* originLongitudeDeg, 
		const double* originAltitude,
		const SPointNED* nedPoint
	);
}
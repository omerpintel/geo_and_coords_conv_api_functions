#pragma once
#include <cstdint>

#pragma pack(push,1)

struct SPointGeo {
	double latitudeDeg;  // deg
	double longitudeDeg; // deg
	double altitude;	 // m
};

struct SPointECEF {
	double x; // m
	double y; // m
	double z; // m
};

struct SPointNED {
	double north; // m
	double east; // m
	double down; // m
};

/**
 * @struct SPointNE
 * @brief Represents a point in a Local Tangent Plane (NED) coordinate system.
 *
 * This structure uses Cartesian coordinates in meters, where 'north' corresponds
 * to the X-axis and 'east' to the Y-axis relative to a local origin.
 */
struct SPointNE {
	float north; /**< Distance in meters along the North axis (X). */
	float east;  /**< Distance in meters along the East axis (Y). */
};

/**
 * @enum EIsInsideResult
 * @brief Result state for isInsidePolygon
 */
enum EIsInsideResult : uint8_t
{
	IS_INSIDE_OK = 0,
	IS_INSIDE_POLYGON_IS_NULL_PTR = 1,
	IS_INSIDE_POLYGON_WITH_LESS_THAN_3_POINTS = 2,
	IS_INSIDE_OUTPUT_PTR_IS_NULL = 3
};

/**
 * @enum ELineIntersectResult
 * @brief Result state for doesLineIntersectPolygon
 */
enum ELineIntersectResult : uint8_t
{
	LINE_INTERSECT_OK = 0,
	LINE_INTERSECT_POLYGON_IS_NULL_PTR = 1,
	LINE_INTERSECT_POLYGON_WITH_LESS_THAN_3_POINTS = 2,
	LINE_INTERSECT_MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO = 3,
	LINE_INTERSECT_OUTPUT_PTR_IS_NULL = 4
};

#pragma pack(pop)
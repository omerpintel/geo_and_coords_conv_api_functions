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
 * @enum ResultState
 * @brief determines the status of the result
 */
enum EResultState : uint8_t
{
	OK = 0,
	POLYGON_WITH_LESS_THAN_3_POINTS = 1,
	POLYGON_IS_NULL_PTR = 2,
	MAX_LENGTH_LESS_OR_EQUAL_TO_ZERO = 3
};

#pragma pack(pop)
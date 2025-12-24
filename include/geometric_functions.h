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


// --- Helper Functions ---
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

bool areAlmostEqual(const float a, const float b);

double getDistSq(const Point& a, const Point& b);

double getDistToSegmentSquared(const Point& p, const Point& a, const Point& b);

bool onSegment(const Point& p, const Point& q, const Point& r);

int orientation(const Point& p, const Point& q, const Point& r);

bool doSegmentsIntersect(const Point& p1, const Point& q1, const Point& p2, const Point& q2);
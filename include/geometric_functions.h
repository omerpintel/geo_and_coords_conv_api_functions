#pragma once

#include "api_structs.h"

#include <cmath>     // for fabs,sin,cos
#include <algorithm> // for std::min, std::max

// --- Constants ---
constexpr double EPSILON = 1e-9;

// --- Helper Functions ---

bool areAlmostEqual(const double a, const double b);

double getDistSq(const SPointNE& a, const SPointNE& b);

double getDistToSegmentSquared(const SPointNE& p, const SPointNE& a, const SPointNE& b);

bool onSegment(const SPointNE& p, const SPointNE& q, const SPointNE& r);

int orientation(const SPointNE& p, const SPointNE& q, const SPointNE& r);

bool doSegmentsIntersect(const SPointNE& p1, const SPointNE& q1, const SPointNE& p2, const SPointNE& q2);
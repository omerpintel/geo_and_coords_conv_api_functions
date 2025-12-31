#pragma once

#include "api_structs.h"

#include <cmath>     // for fabs,sin,cos

// --- Constants ---
const double EPSILON = 1e-9;

// --- Helper Functions ---
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

bool areAlmostEqual(const float a, const float b);

double getDistSq(const SPointNE& a, const SPointNE& b);

double getDistToSegmentSquared(const SPointNE& p, const SPointNE& a, const SPointNE& b);

bool onSegment(const SPointNE& p, const SPointNE& q, const SPointNE& r);

int orientation(const SPointNE& p, const SPointNE& q, const SPointNE& r);

bool doSegmentsIntersect(const SPointNE& p1, const SPointNE& q1, const SPointNE& p2, const SPointNE& q2);
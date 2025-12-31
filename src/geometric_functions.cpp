#include "geometric_functions.h"


// Checks if two double values are effectively equal.
// Uses a scaled epsilon comparison to handle floating-point precision errors.
bool areAlmostEqual(const float a, const float b) {
    return std::fabs(a - b) <= EPSILON * 100.0f;
}

// Calculates the squared Euclidean distance between two points.
// Using squared distance avoids expensive square root operations during comparisons.
double getDistSq(const SPointNE& a, const SPointNE& b) {
    double dn = a.north - b.north;
    double de = a.east - b.east;
    return dn * dn + de * de;
}

// Calculates the squared shortest distance
// from a point to a line segment.
double getDistToSegmentSquared(const SPointNE& p, const SPointNE& a, const SPointNE& b) {
    const float l2 = getDistSq(a, b);

    // If start and end points are identical, return distance to point 'a'
    if (l2 == 0.0) return getDistSq(p, a);

    // Calculate projection factor t represents the relative position of the projection on the infinite line:
    // 0.0 = Start (a), 1.0 = End (b).
    // t = [(p-a) . (b-a)] / |b-a|^2
    float t = ((p.north - a.north) * (b.north - a.north) +
        (p.east - a.east) * (b.east - a.east)) / l2;

    // Clamp t to the segment [0, 1] to handle points beyond endpoints
    if (t < 0.0) t = 0.0;
    else if (t > 1.0) t = 1.0;

    SPointNE projection = {
        a.north + t * (b.north - a.north),
        a.east + t * (b.east - a.east)
    };

    return getDistSq(p, projection);
}

// Checks if point q lies on the line segment pr.
// Assumes points are already known to be collinear.
bool onSegment(const SPointNE& p, const SPointNE& q, const SPointNE& r) {
    return q.north <= MAX(p.north, r.north) && q.north >= MIN(p.north, r.north) &&
        q.east <= MAX(p.east, r.east) && q.east >= MIN(p.east, r.east);
}

// Determines the orientation of the ordered triplet (p, q, r).
 // return 0 if collinear, 1 if clockwise, 2 if counter-clockwise.
int orientation(const SPointNE& p, const SPointNE& q, const SPointNE& r) {
    double val = (q.east - p.east) * (r.north - q.north) -
        (q.north - p.north) * (r.east - q.east);

    if (areAlmostEqual(val, 0.0)) return 0;
    return (val > 0) ? 1 : 2;
}

// Checks if two line segments (p1-q1 and p2-q2) intersect.
// Uses the general case and special cases (collinear points) of the orientation method.
bool doSegmentsIntersect(const SPointNE& p1, const SPointNE& q1, const SPointNE& p2, const SPointNE& q2) {
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    // General Case: Segments straddle each other
    if (o1 != o2 && o3 != o4) return true;

    // Special Cases: Collinear points lying on segments
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;

    return false;
}

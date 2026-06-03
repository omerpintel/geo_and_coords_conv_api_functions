#include "spherical_functions.h"

#include <algorithm> // for std::clamp

double AngularDistanceRad(const Vec3& a, const Vec3& b)
{
    const double c = std::clamp(Vec3Dot(a, b), -1.0, 1.0);
    return std::acos(c);
}

bool IsPointOnGreatCircleSegment(const Vec3& edge_start, const Vec3& edge_end, const Vec3& point, double eps_rad)
{
    // The great-circle plane normal is edge_start x edge_end
    Vec3 normal = Vec3Cross(edge_start, edge_end);
    const double normal_norm = Vec3Norm(normal);

    // Degenerate edge (identical or antipodal endpoints)
    if (normal_norm < 1e-18) return false;

    normal = { normal[0] / normal_norm, normal[1] / normal_norm, normal[2] / normal_norm };

    // Point must lie on (or very close to) the great-circle plane
    if (std::abs(Vec3Dot(point, normal)) > std::sin(eps_rad)) return false;

    // Check that point lies between start and end on the shorter arc
    const double edge_distance = AngularDistanceRad(edge_start, edge_end);
    const double start_to_point = AngularDistanceRad(edge_start, point);
    const double point_to_end = AngularDistanceRad(point, edge_end);

    return std::abs((start_to_point + point_to_end) - edge_distance) <= 2.0 * eps_rad;
}

bool AccumulateEdgeAngle(const Vec3& query_point, const Vec3& edge_start, const Vec3& edge_end, double eps_rad, long double& sum_signed_angles)
{
    // Boundary check: if query is on this edge, treat as inside
    if (IsPointOnGreatCircleSegment(edge_start, edge_end, query_point, eps_rad)) return true;

    // Project both edge endpoints onto the tangent plane at query_point
    const double start_proj = Vec3Dot(query_point, edge_start);
    const double end_proj = Vec3Dot(query_point, edge_end);

    Vec3 u_start = {
        edge_start[0] - start_proj * query_point[0],
        edge_start[1] - start_proj * query_point[1],
        edge_start[2] - start_proj * query_point[2]
    };

    Vec3 u_end = {
        edge_end[0] - end_proj * query_point[0],
        edge_end[1] - end_proj * query_point[1],
        edge_end[2] - end_proj * query_point[2]
    };

    // Normalize projected vectors
    const double u_start_norm = Vec3Norm(u_start);
    const double u_end_norm = Vec3Norm(u_end);

    if (u_start_norm < 1e-18 || u_end_norm < 1e-18) return false;

    u_start = { u_start[0] / u_start_norm, u_start[1] / u_start_norm, u_start[2] / u_start_norm };
    u_end = { u_end[0] / u_end_norm, u_end[1] / u_end_norm, u_end[2] / u_end_norm };

    // Compute signed turning angle
    const Vec3 cross_projected = Vec3Cross(u_start, u_end);
    const double sin_theta = Vec3Dot(query_point, cross_projected);
    const double cos_theta = Vec3Dot(u_start, u_end);

    sum_signed_angles += std::atan2(sin_theta, cos_theta);
    return false;
}

double CrossTrackDistanceMeters(const Vec3& point, const Vec3& arc_start, const Vec3& arc_end)
{
    // Great-circle plane normal for the arc
    Vec3 arc_normal = Vec3Cross(arc_start, arc_end);
    const double arc_normal_norm = Vec3Norm(arc_normal);

    // Degenerate arc: distance is just point-to-point
    if (arc_normal_norm < 1e-18)
    {
        return AngularDistanceRad(point, arc_start) * SphericalConsts::EARTH_RADIUS_M;
    }

    // Cross-track angular distance: arcsin(point . normal_hat)
    Vec3 normal_hat = { arc_normal[0] / arc_normal_norm, arc_normal[1] / arc_normal_norm, arc_normal[2] / arc_normal_norm };
    double cross_track_sin = Vec3Dot(point, normal_hat);
    cross_track_sin = std::clamp(cross_track_sin, -1.0, 1.0);
    double cross_track_rad = std::abs(std::asin(cross_track_sin));

    // Check if the closest point on the great circle falls within the arc segment.
    // Project the query point onto the great circle to find the closest point.
    // The along-track position is checked by comparing angular distances.
    const double edge_distance = AngularDistanceRad(arc_start, arc_end);
    const double start_to_point = AngularDistanceRad(arc_start, point);
    const double point_to_end = AngularDistanceRad(point, arc_end);

    // If the "projection" falls outside the arc, use endpoint distance instead
    // This is a simplified check: if start_to_point or point_to_end exceeds edge_distance,
    // the closest point is one of the endpoints.
    if (start_to_point > edge_distance + SphericalConsts::EPS_RAD ||
        point_to_end > edge_distance + SphericalConsts::EPS_RAD)
    {
        // Closest point is one of the endpoints
        double d_start = AngularDistanceRad(point, arc_start);
        double d_end = AngularDistanceRad(point, arc_end);
        double min_d = (d_start < d_end) ? d_start : d_end;
        return min_d * SphericalConsts::EARTH_RADIUS_M;
    }

    return cross_track_rad * SphericalConsts::EARTH_RADIUS_M;
}

bool DoSphericalArcsIntersect(const Vec3& a1, const Vec3& a2, const Vec3& b1, const Vec3& b2, double eps_rad)
{
    // Normal vectors of the two great-circle planes
    Vec3 n1 = Vec3Cross(a1, a2);
    Vec3 n2 = Vec3Cross(b1, b2);

    double n1_norm = Vec3Norm(n1);
    double n2_norm = Vec3Norm(n2);

    // Degenerate arcs
    if (n1_norm < 1e-18 || n2_norm < 1e-18) return false;

    // The intersection line of the two great-circle planes
    Vec3 intersection = Vec3Cross(n1, n2);
    double int_norm = Vec3Norm(intersection);

    // Parallel great circles (no intersection or identical)
    if (int_norm < 1e-18) return false;

    // Two candidate intersection points (antipodal)
    Vec3 candidate1 = { intersection[0] / int_norm, intersection[1] / int_norm, intersection[2] / int_norm };
    Vec3 candidate2 = { -candidate1[0], -candidate1[1], -candidate1[2] };

    // Check if either candidate lies on both arc segments
    if (IsPointOnGreatCircleSegment(a1, a2, candidate1, eps_rad) &&
        IsPointOnGreatCircleSegment(b1, b2, candidate1, eps_rad))
    {
        return true;
    }

    if (IsPointOnGreatCircleSegment(a1, a2, candidate2, eps_rad) &&
        IsPointOnGreatCircleSegment(b1, b2, candidate2, eps_rad))
    {
        return true;
    }

    return false;
}

void DestinationPointDeg(double startLatDeg, double startLonDeg, double azimuthDeg, double distanceMeters, double& outLatDeg, double& outLonDeg)
{
    const double lat1 = DegToRad(startLatDeg);
    const double lon1 = DegToRad(startLonDeg);
    const double brng = DegToRad(azimuthDeg);
    const double d_over_R = distanceMeters / SphericalConsts::EARTH_RADIUS_M;

    const double sin_lat1 = std::sin(lat1);
    const double cos_lat1 = std::cos(lat1);
    const double sin_dR = std::sin(d_over_R);
    const double cos_dR = std::cos(d_over_R);

    const double lat2 = std::asin(sin_lat1 * cos_dR + cos_lat1 * sin_dR * std::cos(brng));
    const double lon2 = lon1 + std::atan2(
        std::sin(brng) * sin_dR * cos_lat1,
        cos_dR - sin_lat1 * std::sin(lat2)
    );

    outLatDeg = RadToDeg(lat2);
    outLonDeg = RadToDeg(lon2);
}

#pragma once

#include "manifold.hpp"
#include "shapes.hpp"
#include "vec2.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>
#include <vector>

namespace gamedev::physics2d {

class CollisionDetector {
public:
    static bool test_circle_circle(const CircleShape& circleA, Vec2 posA,
                                   const CircleShape& circleB, Vec2 posB,
                                   Manifold& out_manifold) {
        out_manifold.reset();
        Vec2 delta = posB - posA;
        float dist_sq = delta.length_squared();
        float radius_sum = circleA.radius + circleB.radius;

        if (dist_sq >= radius_sum * radius_sum) {
            return false;
        }

        float dist = std::sqrt(dist_sq);
        out_manifold.is_colliding = true;

        if (dist > 1e-6f) {
            out_manifold.normal = delta / dist;
            out_manifold.penetration_depth = radius_sum - dist;
        } else {
            // Concentric circles: pick arbitrary normal
            out_manifold.normal = {0.0f, 1.0f};
            out_manifold.penetration_depth = circleA.radius;
        }

        out_manifold.contact_point = posA + out_manifold.normal * circleA.radius;
        return true;
    }

    /**
     * Projects a set of polygon vertices onto an arbitrary axis.
     */
    static void project_polygon(const std::vector<Vec2>& vertices, Vec2 axis,
                                float& out_min, float& out_max) {
        out_min = axis.dot(vertices[0]);
        out_max = out_min;
        for (size_t i = 1; i < vertices.size(); ++i) {
            float projection = axis.dot(vertices[i]);
            out_min = std::min(out_min, projection);
            out_max = std::max(out_max, projection);
        }
    }

    /**
     * Separating Axis Theorem (SAT) collision test between two convex polygons.
     */
    static bool test_polygon_polygon(const PolygonShape& polyA, Vec2 posA, float rotA,
                                     const PolygonShape& polyB, Vec2 posB, float rotB,
                                     Manifold& out_manifold) {
        out_manifold.reset();

        auto vertsA = polyA.get_transformed_vertices(posA, rotA);
        auto vertsB = polyB.get_transformed_vertices(posB, rotB);
        auto normalsA = polyA.get_transformed_normals(rotA);
        auto normalsB = polyB.get_transformed_normals(rotB);

        float min_overlap = std::numeric_limits<float>::max();
        Vec2 smallest_axis{0.0f, 0.0f};

        // Combine all candidate axes from both polygons
        std::vector<Vec2> axes;
        axes.reserve(normalsA.size() + normalsB.size());
        for (const auto& n : normalsA) axes.push_back(n);
        for (const auto& n : normalsB) axes.push_back(n);

        for (const auto& axis : axes) {
            float minA, maxA, minB, maxB;
            project_polygon(vertsA, axis, minA, maxA);
            project_polygon(vertsB, axis, minB, maxB);

            // Check for separating axis (gap between 1D projections)
            if (minA >= maxB || minB >= maxA) {
                return false; // Separating axis found: shapes do NOT intersect
            }

            // Compute 1D interval overlap depth
            float overlap = std::min(maxA, maxB) - std::max(minA, minB);
            if (overlap < min_overlap) {
                min_overlap = overlap;
                smallest_axis = axis;
            }
        }

        out_manifold.is_colliding = true;
        out_manifold.penetration_depth = min_overlap;

        // Ensure normal vector points from body A towards body B
        Vec2 centerA = posA;
        Vec2 centerB = posB;
        if ((centerB - centerA).dot(smallest_axis) < 0.0f) {
            smallest_axis = -smallest_axis;
        }
        out_manifold.normal = smallest_axis;
        out_manifold.contact_point = posA + out_manifold.normal * (min_overlap * 0.5f);

        return true;
    }

    /**
     * SAT collision test between a Circle and a Convex Polygon.
     */
    static bool test_circle_polygon(const CircleShape& circle, Vec2 posC,
                                    const PolygonShape& poly, Vec2 posP, float rotP,
                                    Manifold& out_manifold) {
        out_manifold.reset();

        auto vertsP = poly.get_transformed_vertices(posP, rotP);
        auto normalsP = poly.get_transformed_normals(rotP);

        // Find vertex on polygon closest to circle center
        float closest_dist_sq = std::numeric_limits<float>::max();
        Vec2 closest_vertex{0.0f, 0.0f};
        for (const auto& v : vertsP) {
            float dist_sq = (posC - v).length_squared();
            if (dist_sq < closest_dist_sq) {
                closest_dist_sq = dist_sq;
                closest_vertex = v;
            }
        }

        std::vector<Vec2> axes = normalsP;
        Vec2 circle_axis = (closest_vertex - posC).normalized();
        if (circle_axis.length_squared() > 1e-6f) {
            axes.push_back(circle_axis);
        }

        float min_overlap = std::numeric_limits<float>::max();
        Vec2 smallest_axis{0.0f, 0.0f};

        for (const auto& axis : axes) {
            float minP, maxP;
            project_polygon(vertsP, axis, minP, maxP);

            float center_proj = axis.dot(posC);
            float minC = center_proj - circle.radius;
            float maxC = center_proj + circle.radius;

            if (minC >= maxP || minP >= maxC) {
                return false;
            }

            float overlap = std::min(maxC, maxP) - std::max(minC, minP);
            if (overlap < min_overlap) {
                min_overlap = overlap;
                smallest_axis = axis;
            }
        }

        out_manifold.is_colliding = true;
        out_manifold.penetration_depth = min_overlap;

        // Ensure normal points from Circle (A) to Polygon (B)
        if ((posP - posC).dot(smallest_axis) < 0.0f) {
            smallest_axis = -smallest_axis;
        }
        out_manifold.normal = smallest_axis;
        out_manifold.contact_point = posC + out_manifold.normal * circle.radius;

        return true;
    }
};

} // namespace gamedev::physics2d

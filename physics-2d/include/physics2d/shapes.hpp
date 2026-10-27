#pragma once

#include "aabb.hpp"
#include "vec2.hpp"

#include <algorithm>
#include <vector>

namespace gamedev::physics2d {

enum class ShapeType {
    Circle,
    Polygon
};

class Shape {
public:
    virtual ~Shape() = default;
    virtual ShapeType type() const noexcept = 0;
    virtual AABB compute_aabb(Vec2 position, float rotation) const = 0;
};

class CircleShape : public Shape {
public:
    float radius = 1.0f;

    explicit CircleShape(float radius_ = 1.0f) : radius(radius_) {}

    ShapeType type() const noexcept override { return ShapeType::Circle; }

    AABB compute_aabb(Vec2 position, float /*rotation*/) const override {
        return AABB(position - Vec2(radius, radius), position + Vec2(radius, radius));
    }
};

class PolygonShape : public Shape {
public:
    std::vector<Vec2> vertices; // Local CCW vertices
    std::vector<Vec2> normals;  // Outward unit edge normals

    PolygonShape() = default;
    explicit PolygonShape(std::vector<Vec2> verts) {
        set_vertices(std::move(verts));
    }

    ShapeType type() const noexcept override { return ShapeType::Polygon; }

    void set_vertices(std::vector<Vec2> verts) {
        vertices = std::move(verts);
        normals.clear();
        normals.reserve(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i) {
            size_t next = (i + 1) % vertices.size();
            Vec2 edge = vertices[next] - vertices[i];
            // Outward normal in 2D for CCW vertices: (edge.y, -edge.x)
            Vec2 norm = Vec2(edge.y, -edge.x).normalized();
            normals.push_back(norm);
        }
    }

    static PolygonShape make_box(float half_width, float half_height) {
        return PolygonShape({
            {-half_width, -half_height},
            { half_width, -half_height},
            { half_width,  half_height},
            {-half_width,  half_height}
        });
    }

    std::vector<Vec2> get_transformed_vertices(Vec2 position, float rotation) const {
        std::vector<Vec2> transformed;
        transformed.reserve(vertices.size());
        for (const auto& v : vertices) {
            transformed.push_back(position + v.rotated(rotation));
        }
        return transformed;
    }

    std::vector<Vec2> get_transformed_normals(float rotation) const {
        std::vector<Vec2> transformed;
        transformed.reserve(normals.size());
        for (const auto& n : normals) {
            transformed.push_back(n.rotated(rotation));
        }
        return transformed;
    }

    AABB compute_aabb(Vec2 position, float rotation) const override {
        auto transformed = get_transformed_vertices(position, rotation);
        if (transformed.empty()) {
            return AABB(position, position);
        }
        Vec2 min_pt = transformed[0];
        Vec2 max_pt = transformed[0];
        for (size_t i = 1; i < transformed.size(); ++i) {
            min_pt.x = std::min(min_pt.x, transformed[i].x);
            min_pt.y = std::min(min_pt.y, transformed[i].y);
            max_pt.x = std::max(max_pt.x, transformed[i].x);
            max_pt.y = std::max(max_pt.y, transformed[i].y);
        }
        return AABB(min_pt, max_pt);
    }
};

} // namespace gamedev::physics2d

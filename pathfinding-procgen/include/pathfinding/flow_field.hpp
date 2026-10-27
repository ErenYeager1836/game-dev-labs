#pragma once

#include "grid_map.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

namespace gamedev::pathfinding {

struct FlowVector {
    float x = 0.0f;
    float y = 0.0f;

    constexpr FlowVector() = default;
    constexpr FlowVector(float x_, float y_) : x(x_), y(y_) {}

    float length_squared() const noexcept { return x * x + y * y; }
    float length() const noexcept { return std::sqrt(length_squared()); }
};

/**
 * Flow Field (Vector Field) pathfinding engine.
 * Generates an integration distance field from a target via Dijkstra/BFS flood-fill,
 * then constructs 2D direction vectors for every grid cell pointing down the cost gradient.
 * Provides O(1) instantaneous navigation queries for hundreds or thousands of concurrent units.
 */
class FlowField {
public:
    static constexpr float INFINITE_COST = std::numeric_limits<float>::infinity();

    FlowField(const GridMap& map, Point target)
        : width_(map.width()), height_(map.height()), target_(target) {
        generate(map, target);
    }

    void generate(const GridMap& map, Point target) {
        width_ = map.width();
        height_ = map.height();
        target_ = target;

        const size_t total_tiles = static_cast<size_t>(width_ * height_);
        cost_field_.assign(total_tiles, INFINITE_COST);
        flow_field_.assign(total_tiles, FlowVector(0.0f, 0.0f));

        if (!map.in_bounds(target) || !map.is_walkable(target)) {
            return;
        }

        // 1. Calculate Integration Cost Field using BFS/Dijkstra flood fill from target
        std::queue<Point> queue;
        cost_field_[map.to_index(target)] = 0.0f;
        queue.push(target);

        static const int dx[] = { 0, 1, 0, -1,  1, 1, -1, -1 };
        static const int dy[] = { -1, 0, 1, 0, -1, 1,  1, -1 };
        static const float step_cost[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.41421356f, 1.41421356f, 1.41421356f, 1.41421356f };

        while (!queue.empty()) {
            Point curr = queue.front();
            queue.pop();

            float curr_cost = cost_field_[map.to_index(curr)];

            for (int i = 0; i < 8; ++i) {
                int nx = curr.x + dx[i];
                int ny = curr.y + dy[i];
                Point nbr(nx, ny);

                if (!map.in_bounds(nbr) || !map.is_walkable(nbr)) {
                    continue;
                }

                // Prevent diagonal corner cutting if adjacent orthos are walls
                if (i >= 4) {
                    if (!map.is_walkable(curr.x, ny) || !map.is_walkable(nx, curr.y)) {
                        continue;
                    }
                }

                float tentative_cost = curr_cost + step_cost[i];
                size_t nbr_idx = map.to_index(nbr);

                if (tentative_cost < cost_field_[nbr_idx]) {
                    cost_field_[nbr_idx] = tentative_cost;
                    queue.push(nbr);
                }
            }
        }

        // 2. Compute Flow Vector Field by looking at neighbors with lowest integration cost
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                Point p(x, y);
                size_t idx = map.to_index(p);

                if (p == target_) {
                    flow_field_[idx] = FlowVector(0.0f, 0.0f);
                    continue;
                }

                if (!map.is_walkable(p) || cost_field_[idx] == INFINITE_COST) {
                    flow_field_[idx] = FlowVector(0.0f, 0.0f);
                    continue;
                }

                float min_cost = cost_field_[idx];
                int best_dx = 0;
                int best_dy = 0;

                for (int i = 0; i < 8; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    Point nbr(nx, ny);

                    if (!map.in_bounds(nbr) || !map.is_walkable(nbr)) {
                        continue;
                    }

                    if (i >= 4) {
                        if (!map.is_walkable(x, ny) || !map.is_walkable(nx, y)) {
                            continue;
                        }
                    }

                    float nbr_cost = cost_field_[map.to_index(nbr)];
                    if (nbr_cost < min_cost) {
                        min_cost = nbr_cost;
                        best_dx = dx[i];
                        best_dy = dy[i];
                    }
                }

                if (best_dx != 0 || best_dy != 0) {
                    float len = std::sqrt(static_cast<float>(best_dx * best_dx + best_dy * best_dy));
                    flow_field_[idx] = FlowVector(best_dx / len, best_dy / len);
                }
            }
        }
    }

    Point target() const noexcept { return target_; }

    float get_cost(Point p) const noexcept {
        if (!in_bounds(p)) return INFINITE_COST;
        return cost_field_[to_index(p)];
    }

    FlowVector get_flow(Point p) const noexcept {
        if (!in_bounds(p)) return FlowVector(0.0f, 0.0f);
        return flow_field_[to_index(p)];
    }

    bool is_reachable(Point p) const noexcept {
        return get_cost(p) < INFINITE_COST;
    }

    std::vector<Point> trace_path(Point start, size_t max_steps = 1000) const {
        std::vector<Point> path;
        if (!is_reachable(start)) {
            return path;
        }

        Point curr = start;
        path.push_back(curr);

        while (curr != target_ && path.size() < max_steps) {
            FlowVector v = get_flow(curr);
            if (v.length_squared() < 1e-4f) {
                break;
            }

            int step_x = (v.x > 0.3f) ? 1 : ((v.x < -0.3f) ? -1 : 0);
            int step_y = (v.y > 0.3f) ? 1 : ((v.y < -0.3f) ? -1 : 0);
            Point next(curr.x + step_x, curr.y + step_y);

            if (next == curr || !in_bounds(next)) {
                break;
            }

            curr = next;
            path.push_back(curr);
        }

        return path;
    }

private:
    bool in_bounds(Point p) const noexcept {
        return p.x >= 0 && p.x < width_ && p.y >= 0 && p.y < height_;
    }

    size_t to_index(Point p) const noexcept {
        return static_cast<size_t>(p.y * width_ + p.x);
    }

    int width_ = 0;
    int height_ = 0;
    Point target_;
    std::vector<float> cost_field_;
    std::vector<FlowVector> flow_field_;
};

} // namespace gamedev::pathfinding

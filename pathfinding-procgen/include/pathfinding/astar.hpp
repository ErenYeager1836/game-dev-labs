#pragma once

#include "grid_map.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace gamedev::pathfinding {

class AStar {
public:
    struct SearchResult {
        bool path_found = false;
        std::vector<Point> path;
        size_t nodes_explored = 0;
        float total_cost = 0.0f;
    };

    /**
     * Executes the A* pathfinding search from start to goal.
     */
    static SearchResult find_path(const GridMap& map, Point start, Point goal, bool allow_diagonal = false) {
        SearchResult result;

        if (!map.in_bounds(start) || !map.in_bounds(goal)) {
            return result;
        }

        if (!map.is_walkable(start) || !map.is_walkable(goal)) {
            return result;
        }

        if (start == goal) {
            result.path_found = true;
            result.path = {start};
            return result;
        }

        const size_t total_tiles = static_cast<size_t>(map.width() * map.height());
        std::vector<float> g_cost(total_tiles, std::numeric_limits<float>::infinity());
        std::vector<Point> parent(total_tiles, Point(-1, -1));
        std::vector<bool> closed(total_tiles, false);

        // Priority queue node storing (f_score, h_score, Point)
        struct PQNode {
            float f_cost;
            float h_cost;
            Point pos;

            bool operator>(const PQNode& o) const noexcept {
                // Lower f_cost has higher priority. Tie-break using lower h_cost.
                if (std::abs(f_cost - o.f_cost) > 1e-5f) {
                    return f_cost > o.f_cost;
                }
                return h_cost > o.h_cost;
            }
        };

        std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> open_set;

        size_t start_idx = map.to_index(start);
        g_cost[start_idx] = 0.0f;
        float start_h = heuristic(start, goal, allow_diagonal);
        open_set.push({start_h, start_h, start});

        // 4-way cardinal directions (or 8-way with diagonals)
        const std::vector<Point> neighbors_4 = {
            {0, -1}, {0, 1}, {-1, 0}, {1, 0}
        };
        const std::vector<Point> neighbors_8 = {
            {0, -1}, {0, 1}, {-1, 0}, {1, 0},
            {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
        };
        const auto& dirs = allow_diagonal ? neighbors_8 : neighbors_4;

        while (!open_set.empty()) {
            PQNode current = open_set.top();
            open_set.pop();

            Point current_pos = current.pos;
            size_t current_idx = map.to_index(current_pos);

            if (closed[current_idx]) {
                continue;
            }
            closed[current_idx] = true;
            result.nodes_explored++;

            if (current_pos == goal) {
                // Goal reached! Reconstruct shortest path.
                result.path_found = true;
                result.total_cost = g_cost[current_idx];

                Point curr = goal;
                size_t safety_counter = 0;
                while (curr != start && map.in_bounds(curr) && safety_counter++ < total_tiles) {
                    result.path.push_back(curr);
                    size_t idx = map.to_index(curr);
                    if (idx >= parent.size() || !map.in_bounds(parent[idx])) {
                        break;
                    }
                    curr = parent[idx];
                }
                result.path.push_back(start);
                std::reverse(result.path.begin(), result.path.end());
                return result;
            }

            for (const auto& dir : dirs) {
                Point neighbor(current_pos.x + dir.x, current_pos.y + dir.y);

                if (!map.in_bounds(neighbor) || !map.is_walkable(neighbor)) {
                    continue;
                }

                size_t neighbor_idx = map.to_index(neighbor);
                if (closed[neighbor_idx]) {
                    continue;
                }

                // Prevent corner cutting in diagonal movement
                if (dir.x != 0 && dir.y != 0) {
                    if (!map.is_walkable(current_pos.x + dir.x, current_pos.y) ||
                        !map.is_walkable(current_pos.x, current_pos.y + dir.y)) {
                        continue;
                    }
                }

                float step_cost = (dir.x != 0 && dir.y != 0) ? 1.41421356f : 1.0f;
                float tentative_g = g_cost[current_idx] + step_cost;

                if (tentative_g < g_cost[neighbor_idx]) {
                    g_cost[neighbor_idx] = tentative_g;
                    parent[neighbor_idx] = current_pos;
                    float h = heuristic(neighbor, goal, allow_diagonal);
                    open_set.push({tentative_g + h, h, neighbor});
                }
            }
        }

        // Exhausted open set without finding goal
        return result;
    }

    /**
     * Admissible heuristic estimator:
     * - Manhattan Distance for 4-directional grids (dx + dy)
     * - Octile Distance for 8-directional grids
     */
    static float heuristic(Point a, Point b, bool allow_diagonal) noexcept {
        int dx = std::abs(a.x - b.x);
        int dy = std::abs(a.y - b.y);

        if (!allow_diagonal) {
            return static_cast<float>(dx + dy);
        }

        // Octile distance
        int min_d = std::min(dx, dy);
        int max_d = std::max(dx, dy);
        return static_cast<float>(max_d) + 0.41421356f * static_cast<float>(min_d);
    }
};

} // namespace gamedev::pathfinding

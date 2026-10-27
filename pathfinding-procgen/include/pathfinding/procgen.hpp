#pragma once

#include "grid_map.hpp"

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

namespace gamedev::pathfinding {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr Rect() = default;
    constexpr Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), width(w_), height(h_) {}

    constexpr Point center() const noexcept {
        return Point(x + width / 2, y + height / 2);
    }

    constexpr bool intersects(const Rect& o, int padding = 0) const noexcept {
        return x - padding < o.x + o.width && x + width + padding > o.x &&
               y - padding < o.y + o.height && y + height + padding > o.y;
    }
};

class ProcGen {
public:
    /**
     * Generates a fully-connected dungeon using Binary Space Partitioning (BSP).
     */
    static std::vector<Rect> generate_bsp_dungeon(GridMap& map, unsigned int seed = 42,
                                                  int min_room_size = 5, int max_rooms = 12) {
        map.fill(TileType::Wall);
        std::mt19937 rng(seed);
        std::vector<Rect> rooms;

        // BSP Partition node
        struct BSPNode {
            Rect area;
            std::unique_ptr<BSPNode> left;
            std::unique_ptr<BSPNode> right;
            Rect room;

            explicit BSPNode(Rect r) : area(r) {}

            bool split(int min_size, std::mt19937& r) {
                if (left || right) return false; // Already split

                bool split_h = (r() % 2 == 0);
                if (area.width > area.height && area.width / area.height >= 1.25) split_h = false;
                else if (area.height > area.width && area.height / area.width >= 1.25) split_h = true;

                int max_dim = split_h ? area.height : area.width;
                if (max_dim < min_size * 2 + 2) return false;

                std::uniform_int_distribution<int> dist(min_size + 1, max_dim - min_size - 1);
                int split_pos = dist(r);

                if (split_h) {
                    left = std::make_unique<BSPNode>(Rect(area.x, area.y, area.width, split_pos));
                    right = std::make_unique<BSPNode>(Rect(area.x, area.y + split_pos, area.width, area.height - split_pos));
                } else {
                    left = std::make_unique<BSPNode>(Rect(area.x, area.y, split_pos, area.height));
                    right = std::make_unique<BSPNode>(Rect(area.x + split_pos, area.y, area.width - split_pos, area.height));
                }
                return true;
            }

            void create_rooms(std::vector<Rect>& out_rooms, int min_size, std::mt19937& r) {
                if (left || right) {
                    if (left) left->create_rooms(out_rooms, min_size, r);
                    if (right) right->create_rooms(out_rooms, min_size, r);
                } else {
                    // Leaf node: carve a room padded inside partition
                    int rw = std::uniform_int_distribution<int>(min_size, std::max(min_size, area.width - 2))(r);
                    int rh = std::uniform_int_distribution<int>(min_size, std::max(min_size, area.height - 2))(r);
                    int rx = std::uniform_int_distribution<int>(area.x + 1, area.x + area.width - rw - 1)(r);
                    int ry = std::uniform_int_distribution<int>(area.y + 1, area.y + area.height - rh - 1)(r);

                    room = Rect(rx, ry, rw, rh);
                    out_rooms.push_back(room);
                }
            }
        };

        // Root covering interior map area (1 tile border)
        BSPNode root(Rect(1, 1, map.width() - 2, map.height() - 2));
        std::vector<BSPNode*> nodes_to_split = {&root};

        while (!nodes_to_split.empty() && static_cast<int>(rooms.size() + nodes_to_split.size()) < max_rooms) {
            BSPNode* curr = nodes_to_split.back();
            nodes_to_split.pop_back();

            if (curr->split(min_room_size, rng)) {
                nodes_to_split.push_back(curr->left.get());
                nodes_to_split.push_back(curr->right.get());
            }
        }

        root.create_rooms(rooms, min_room_size, rng);

        // Carve all generated rooms
        for (const auto& r : rooms) {
            for (int y = r.y; y < r.y + r.height; ++y) {
                for (int x = r.x; x < r.x + r.width; ++x) {
                    map.set(x, y, TileType::Floor);
                }
            }
        }

        // Connect adjacent rooms via L-shaped corridors
        for (size_t i = 0; i + 1 < rooms.size(); ++i) {
            Point p1 = rooms[i].center();
            Point p2 = rooms[i + 1].center();

            // Horizontal tunnel
            int x_start = std::min(p1.x, p2.x);
            int x_end = std::max(p1.x, p2.x);
            for (int x = x_start; x <= x_end; ++x) {
                map.set(x, p1.y, TileType::Floor);
            }

            // Vertical tunnel
            int y_start = std::min(p1.y, p2.y);
            int y_end = std::max(p1.y, p2.y);
            for (int y = y_start; y <= y_end; ++y) {
                map.set(p2.x, y, TileType::Floor);
            }
        }

        return rooms;
    }

    /**
     * Generates organic cave maps using Cellular Automata (4-5 smoothing rule).
     */
    static void generate_cellular_cave(GridMap& map, unsigned int seed = 42,
                                       float initial_wall_prob = 0.45f, int iterations = 4) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // 1. Initial random fill
        for (int y = 0; y < map.height(); ++y) {
            for (int x = 0; x < map.width(); ++x) {
                if (x == 0 || x == map.width() - 1 || y == 0 || y == map.height() - 1) {
                    map.set(x, y, TileType::Wall);
                } else {
                    map.set(x, y, (dist(rng) < initial_wall_prob) ? TileType::Wall : TileType::Floor);
                }
            }
        }

        // 2. Iterative cellular automata smoothing steps
        for (int iter = 0; iter < iterations; ++iter) {
            GridMap next = map;
            for (int y = 1; y < map.height() - 1; ++y) {
                for (int x = 1; x < map.width() - 1; ++x) {
                    int wall_neighbors = count_wall_neighbors(map, x, y);

                    // 4-5 Rule: become wall if 5+ wall neighbors, or if 0 neighbors (isolated wall preservation)
                    if (wall_neighbors >= 5 || wall_neighbors == 0) {
                        next.set(x, y, TileType::Wall);
                    } else {
                        next.set(x, y, TileType::Floor);
                    }
                }
            }
            map = next;
        }
    }

private:
    static int count_wall_neighbors(const GridMap& map, int cx, int cy) {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = cx + dx;
                int ny = cy + dy;
                if (!map.in_bounds(nx, ny) || map.get(nx, ny) == TileType::Wall) {
                    count++;
                }
            }
        }
        return count;
    }
};

} // namespace gamedev::pathfinding

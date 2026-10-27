#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace gamedev::pathfinding {

enum class TileType : uint8_t {
    Empty = 0,
    Wall = 1,
    Floor = 2,
    Door = 3,
    Path = 4,
    Start = 5,
    Goal = 6
};

struct Point {
    int x = 0;
    int y = 0;

    constexpr Point() = default;
    constexpr Point(int x_, int y_) : x(x_), y(y_) {}

    constexpr bool operator==(const Point& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Point& o) const noexcept { return !(*this == o); }
    constexpr bool operator<(const Point& o) const noexcept {
        return y == o.y ? x < o.x : y < o.y;
    }
};

class GridMap {
public:
    GridMap(int width, int height, TileType default_tile = TileType::Wall)
        : width_(width), height_(height), tiles_(width * height, default_tile) {}

    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }

    bool in_bounds(int x, int y) const noexcept {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }

    bool in_bounds(Point p) const noexcept {
        return in_bounds(p.x, p.y);
    }

    size_t to_index(int x, int y) const noexcept {
        return static_cast<size_t>(y * width_ + x);
    }

    size_t to_index(Point p) const noexcept {
        return to_index(p.x, p.y);
    }

    Point to_point(size_t index) const noexcept {
        return Point(static_cast<int>(index % width_), static_cast<int>(index / width_));
    }

    TileType get(int x, int y) const {
        if (!in_bounds(x, y)) return TileType::Wall;
        return tiles_[to_index(x, y)];
    }

    TileType get(Point p) const {
        return get(p.x, p.y);
    }

    void set(int x, int y, TileType tile) {
        if (in_bounds(x, y)) {
            tiles_[to_index(x, y)] = tile;
        }
    }

    void set(Point p, TileType tile) {
        set(p.x, p.y, tile);
    }

    bool is_walkable(int x, int y) const noexcept {
        if (!in_bounds(x, y)) return false;
        TileType t = tiles_[to_index(x, y)];
        return t == TileType::Floor || t == TileType::Door || t == TileType::Path ||
               t == TileType::Start || t == TileType::Goal;
    }

    bool is_walkable(Point p) const noexcept {
        return is_walkable(p.x, p.y);
    }

    void fill(TileType tile) {
        std::fill(tiles_.begin(), tiles_.end(), tile);
    }

    char get_glyph(TileType tile) const {
        switch (tile) {
            case TileType::Wall:  return '#';
            case TileType::Floor: return '.';
            case TileType::Door:  return '+';
            case TileType::Path:  return '*';
            case TileType::Start: return 'S';
            case TileType::Goal:  return 'G';
            default:              return ' ';
        }
    }

    void print() const {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                std::cout << get_glyph(get(x, y));
            }
            std::cout << "\n";
        }
    }

private:
    int width_;
    int height_;
    std::vector<TileType> tiles_;
};

} // namespace gamedev::pathfinding

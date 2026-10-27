#include "pathfinding/astar.hpp"
#include "pathfinding/grid_map.hpp"
#include "pathfinding/procgen.hpp"

#include <cassert>
#include <iostream>

using namespace gamedev::pathfinding;

void test_grid_map_basic_operations() {
    std::cout << "[+] Running test_grid_map_basic_operations...\n";
    GridMap map(10, 10, TileType::Wall);

    assert(map.width() == 10);
    assert(map.height() == 10);
    assert(map.in_bounds(0, 0));
    assert(map.in_bounds(9, 9));
    assert(!map.in_bounds(-1, 0));
    assert(!map.in_bounds(10, 5));

    assert(!map.is_walkable(2, 3));
    map.set(2, 3, TileType::Floor);
    assert(map.is_walkable(2, 3));
    assert(map.get(2, 3) == TileType::Floor);

    Point p = map.to_point(map.to_index(4, 7));
    assert(p.x == 4 && p.y == 7);

    std::cout << "    PASSED\n";
}

void test_astar_straight_line_path() {
    std::cout << "[+] Running test_astar_straight_line_path...\n";
    GridMap map(10, 10, TileType::Floor);

    Point start(1, 1);
    Point goal(1, 6);

    auto result = AStar::find_path(map, start, goal, false);
    assert(result.path_found);
    assert(result.path.size() == 6);
    assert(result.path.front() == start);
    assert(result.path.back() == goal);
    assert(result.total_cost == 5.0f);

    std::cout << "    PASSED\n";
}

void test_astar_obstacle_avoidance() {
    std::cout << "[+] Running test_astar_obstacle_avoidance...\n";
    // 7x7 grid with a wall barrier at x = 3, y = [1, 5]
    GridMap map(7, 7, TileType::Floor);

    for (int y = 1; y <= 5; ++y) {
        map.set(3, y, TileType::Wall);
    }

    Point start(1, 3);
    Point goal(5, 3);

    auto result = AStar::find_path(map, start, goal, false);
    assert(result.path_found);

    // Verify none of the path points lie on the wall
    for (const auto& pt : result.path) {
        assert(map.get(pt) != TileType::Wall);
    }

    assert(result.path.front() == start);
    assert(result.path.back() == goal);

    std::cout << "    PASSED\n";
}

void test_astar_unreachable_target() {
    std::cout << "[+] Running test_astar_unreachable_target...\n";
    GridMap map(10, 10, TileType::Floor);

    // Completely surround goal (5, 5) with solid walls
    Point goal(5, 5);
    map.set(4, 5, TileType::Wall);
    map.set(6, 5, TileType::Wall);
    map.set(5, 4, TileType::Wall);
    map.set(5, 6, TileType::Wall);

    Point start(1, 1);
    auto result = AStar::find_path(map, start, goal, false);
    assert(!result.path_found);
    assert(result.path.empty());

    std::cout << "    PASSED\n";
}

void test_astar_diagonal_movement() {
    std::cout << "[+] Running test_astar_diagonal_movement...\n";
    GridMap map(10, 10, TileType::Floor);

    Point start(1, 1);
    Point goal(5, 5);

    auto res_manhattan = AStar::find_path(map, start, goal, false);
    auto res_diagonal = AStar::find_path(map, start, goal, true);

    assert(res_manhattan.path_found && res_diagonal.path_found);
    // Diagonal movement takes 5 steps (including start), while Manhattan takes 9 steps
    assert(res_diagonal.path.size() < res_manhattan.path.size());
    assert(res_diagonal.total_cost < res_manhattan.total_cost);

    std::cout << "    PASSED\n";
}

void test_bsp_dungeon_connectivity() {
    std::cout << "[+] Running test_bsp_dungeon_connectivity...\n";
    GridMap map(40, 20);
    auto rooms = ProcGen::generate_bsp_dungeon(map, 999, 5, 8);

    assert(rooms.size() >= 2);

    // Verify all rooms are interconnected: path must exist from Room 0 to every other room
    Point root_room_center = rooms[0].center();
    assert(map.is_walkable(root_room_center));

    for (size_t i = 1; i < rooms.size(); ++i) {
        Point other_center = rooms[i].center();
        assert(map.is_walkable(other_center));

        auto search = AStar::find_path(map, root_room_center, other_center, false);
        assert(search.path_found && "BSP dungeon must guarantee full room connectivity");
    }

    std::cout << "    PASSED\n";
}

void test_cellular_automata_bounds_and_clusters() {
    std::cout << "[+] Running test_cellular_automata_bounds_and_clusters...\n";
    GridMap map(30, 15);
    ProcGen::generate_cellular_cave(map, 1234, 0.45f, 4);

    // Verify borders are solid walls
    for (int x = 0; x < map.width(); ++x) {
        assert(map.get(x, 0) == TileType::Wall);
        assert(map.get(x, map.height() - 1) == TileType::Wall);
    }
    for (int y = 0; y < map.height(); ++y) {
        assert(map.get(0, y) == TileType::Wall);
        assert(map.get(map.width() - 1, y) == TileType::Wall);
    }

    // Verify there are walkable floor tiles inside
    size_t floor_count = 0;
    for (int y = 1; y < map.height() - 1; ++y) {
        for (int x = 1; x < map.width() - 1; ++x) {
            if (map.get(x, y) == TileType::Floor) {
                floor_count++;
            }
        }
    }
    assert(floor_count > 20);

    std::cout << "    PASSED\n";
}

int main() {
    std::cout << "=== Running pathfinding-procgen test suite ===\n";

    test_grid_map_basic_operations();
    test_astar_straight_line_path();
    test_astar_obstacle_avoidance();
    test_astar_unreachable_target();
    test_astar_diagonal_movement();
    test_bsp_dungeon_connectivity();
    test_cellular_automata_bounds_and_clusters();

    std::cout << "=== All 7 pathfinding-procgen tests passed successfully! ===\n";
    return 0;
}

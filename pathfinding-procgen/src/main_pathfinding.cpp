#include "pathfinding/astar.hpp"
#include "pathfinding/grid_map.hpp"
#include "pathfinding/procgen.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

using namespace gamedev::pathfinding;

void print_ansi_map(const GridMap& map) {
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            TileType t = map.get(x, y);
            switch (t) {
                case TileType::Wall:
                    // Dim gray wall
                    std::cout << "\033[90m#\033[0m";
                    break;
                case TileType::Floor:
                    // Dark green floor
                    std::cout << "\033[32m.\033[0m";
                    break;
                case TileType::Path:
                    // Bright yellow path asterisk
                    std::cout << "\033[1;33m*\033[0m";
                    break;
                case TileType::Start:
                    // Bright green start hero
                    std::cout << "\033[1;32mS\033[0m";
                    break;
                case TileType::Goal:
                    // Bright red goal chest
                    std::cout << "\033[1;31mG\033[0m";
                    break;
                default:
                    std::cout << " ";
                    break;
            }
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "==========================================================================\n";
    std::cout << "  game-dev-labs // Procedural Level Generation & A* Pathfinding Engine    \n";
    std::cout << "      Binary Space Partitioning (BSP) & Optimal Grid Navigation           \n";
    std::cout << "==========================================================================\n\n";

    constexpr int MAP_WIDTH = 48;
    constexpr int MAP_HEIGHT = 18;
    GridMap dungeon(MAP_WIDTH, MAP_HEIGHT);

    // 1. Procedural Generation: BSP Rooms & Corridors
    std::cout << "[*] Generating procedural BSP dungeon (size: " << MAP_WIDTH << "x" << MAP_HEIGHT << ")...\n";
    auto t_gen_start = std::chrono::high_resolution_clock::now();
    auto rooms = ProcGen::generate_bsp_dungeon(dungeon, 1337, 5, 8);
    auto t_gen_end = std::chrono::high_resolution_clock::now();
    double gen_us = std::chrono::duration<double, std::micro>(t_gen_end - t_gen_start).count();

    std::cout << "[+] Carved " << rooms.size() << " connected rooms in "
              << std::fixed << std::setprecision(1) << gen_us << " us.\n\n";

    if (rooms.size() < 2) {
        std::cerr << "[-] Error: Expected at least 2 rooms for pathfinding demonstration.\n";
        return 1;
    }

    // 2. Select Start in First Room and Goal in Last Room
    Point start = rooms.front().center();
    Point goal = rooms.back().center();

    std::cout << "[*] Running A* Pathfinding Search:\n";
    std::cout << "    Start Position : " << start.x << ", " << start.y << "\n";
    std::cout << "    Goal Position  : " << goal.x << ", " << goal.y << "\n";

    auto t_search_start = std::chrono::high_resolution_clock::now();
    auto result = AStar::find_path(dungeon, start, goal, false);
    auto t_search_end = std::chrono::high_resolution_clock::now();
    double search_us = std::chrono::duration<double, std::micro>(t_search_end - t_search_start).count();

    if (!result.path_found) {
        std::cout << "[-] Path could not be found.\n";
        return 1;
    }

    std::cout << "[+] Shortest path discovered in " << std::setprecision(1) << search_us << " us!\n";
    std::cout << "    Path Length    : " << result.path.size() << " steps (Cost: " << result.total_cost << ")\n";
    std::cout << "    Nodes Explored : " << result.nodes_explored << " tiles\n\n";

    // Overlay path on map
    for (const auto& pt : result.path) {
        dungeon.set(pt, TileType::Path);
    }
    dungeon.set(start, TileType::Start);
    dungeon.set(goal, TileType::Goal);

    std::cout << "--- Rendered Procedural Dungeon Map (S = Start, G = Goal, * = A* Path) ---\n";
    print_ansi_map(dungeon);

    // 3. Cellular Automata Demo
    std::cout << "\n[*] Demonstrating Organic Cellular Automata Cave Generation (size: 30x12):\n";
    GridMap cave(30, 12);
    ProcGen::generate_cellular_cave(cave, 777, 0.45f, 4);
    print_ansi_map(cave);

    std::cout << "\n==========================================================================\n";
    std::cout << "Procedural generation and A* navigation concluded successfully.\n";
    std::cout << "==========================================================================\n";

    return 0;
}

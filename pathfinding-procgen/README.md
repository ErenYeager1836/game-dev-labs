# Procedural Level Generation & A* Pathfinding Engine

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../LICENSE)

A modular **Procedural Level Generation & Grid Pathfinding** engine in modern C++17 featuring **Binary Space Partitioning (BSP)** room generation, **Cellular Automata** organic caves, and an optimized **$A^*$ Search Algorithm** with admissible heuristics and tie-breaking.

---

## Key Features

- **$A^*$ Pathfinding Search**:
  - Cardinal (4-way Manhattan) and diagonal (8-way Octile) movement modes.
  - Priority queue tie-breaking on heuristic distance to minimize expanded search space.
  - Strict corner-cutting validation preventing diagonal movement through intersecting wall corners.
  - Provable shortest-path guarantee via admissible heuristics ($h(n) \le h^*(n)$).
- **Procedural Content Generation (PCG)**:
  - **Binary Space Partitioning (BSP)**: Generates structured, aesthetically partitioned dungeons with guaranteed $100\%$ room-to-room connectivity via orthogonal corridors.
  - **Cellular Automata**: Generates natural, organic cave formations using the 4-5 Moore neighborhood simulation rule.
- **Rich Terminal ANSI Map Visualization**: Real-time colorful ASCII map rendering displaying dungeon layouts, start/goal positions, and calculated paths.

---

## Architectural Layout

```text
pathfinding-procgen/
├── CMakeLists.txt
├── DESIGN.md               # Algorithmic notes on A* admissibility & BSP recursion
├── README.md
├── include/
│   └── pathfinding/
│       ├── astar.hpp       # A* search algorithm with priority queue
│       ├── grid_map.hpp    # 2D GridMap tile storage & bounds checking
│       └── procgen.hpp     # BSP dungeon & Cellular Automata cave generators
├── src/
│   └── main_pathfinding.cpp # Interactive CLI procedural map & path demo
└── tests/
    └── test_pathfinding.cpp # 7 automated unit and connectivity test cases
```

---

## Building and Running

### Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Run the Procedural Generation & Pathfinding Demo

```bash
./pathfinding_demo
```

#### Sample Demo Output:
```text
==========================================================================
  game-dev-labs // Procedural Level Generation & A* Pathfinding Engine    
      Binary Space Partitioning (BSP) & Optimal Grid Navigation           
==========================================================================

[*] Generating procedural BSP dungeon (size: 48x18)...
[+] Carved 5 connected rooms in 48.2 us.

[*] Running A* Pathfinding Search:
    Start Position : 5, 4
    Goal Position  : 41, 13
[+] Shortest path discovered in 122.4 us!
    Path Length    : 45 steps (Cost: 44.0)
    Nodes Explored : 118 tiles

--- Rendered Procedural Dungeon Map (S = Start, G = Goal, * = A* Path) ---
################################################
#..........#..................#................#
#..........#..................#................#
#....S*****#..................#................#
#....#....*#..................#................#
#....#....*#..................#................#
#....#....*#..................#................#
#....#....********************#................#
#....#.......................*#................#
#....#.......................*#................#
#....#.......................*#................#
#....#.......................*#................#
#....#.......................*#................#
#....#.......................*************G....#
#....#.......................#............#....#
################################################

[*] Demonstrating Organic Cellular Automata Cave Generation (size: 30x12):
##############################
#..####......#...............#
#...###.......#..............#
#....##........#...##........#
#...............#####........#
#........##.....#####........#
#.......####.....###.........#
#......######................#
#.......####........#........#
#........##........###.......#
#.................#####......#
##############################

==========================================================================
Procedural generation and A* navigation concluded successfully.
==========================================================================
```

### Run Automated Unit Tests

```bash
./pathfinding_tests
```

```text
=== Running pathfinding-procgen test suite ===
[+] Running test_grid_map_basic_operations...
    PASSED
[+] Running test_astar_straight_line_path...
    PASSED
[+] Running test_astar_obstacle_avoidance...
    PASSED
[+] Running test_astar_unreachable_target...
    PASSED
[+] Running test_astar_diagonal_movement...
    PASSED
[+] Running test_bsp_dungeon_connectivity...
    PASSED
[+] Running test_cellular_automata_bounds_and_clusters...
    PASSED
=== All 7 pathfinding-procgen tests passed successfully! ===
```

---

## Design Trade-offs & Limitations

1. **Grid vs Navigation Mesh (NavMesh)**:
   - Grid-based pathfinding uses discrete tiles. While intuitive and simple, open world games often use NavMeshes (convex polygons) with A* to allow continuous movement and arbitrary unit radiuses.
2. **Corridor Carving Strategy**:
   - The current BSP algorithm connects rooms with orthogonal L-shaped corridors. While simple and guaranteed connected, advanced layouts often use Minimum Spanning Trees (Kruskal/Prim) to add loops and alternate routes.

---

## Roadmap

- [x] Discrete 2D GridMap with bounds and walkability checks.
- [x] A* search algorithm with Manhattan and Octile heuristics.
- [x] BSP procedural dungeon generator with guaranteed connectivity.
- [x] Cellular Automata organic cave smoothing.
- [ ] **Jump Point Search (JPS)**: Order-of-magnitude acceleration on uniform-cost grids by skipping intermediate symmetry nodes.
- [ ] **Hierarchical Pathfinding (HPA\*)**: Hierarchical abstraction clustering the grid into chunks for high-performance massive world routing.
- [ ] **Dungeon Loops with Delaunay Triangulation & MST**: Replacing sequential L-corridors with Delaunay graph triangulation and Kruskal's algorithm to generate loops and secret passageways.
- [ ] **Flow Field Pathfinding**: Dijkstra vector flow fields for simultaneous mass-crowd movement (thousands of units pathfinding to a shared goal).

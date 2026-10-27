# Entity Component System (ECS)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../LICENSE)

A high-performance, cache-friendly **Entity Component System (ECS)** in modern C++17 utilizing the **Sparse Set** architectural pattern for constant-time ($O(1)$) operations, contiguous memory cache locality, and multi-component view queries.

---

## Key Features

- **Data-Oriented Design (DoD)**: Separates game state from logic, packing components contiguously in memory for vectorized CPU cache utilization.
- **Sparse Set Storage**:
  - $O(1)$ amortized component addition (`emplace`).
  - $O(1)$ component lookup (`get`, `has`).
  - $O(1)$ component deletion via **swap-and-pop** to preserve packed memory with zero fragmentation.
- **Fast Multi-Component Queries**: `Registry::view<T1, T2, ...>()` runs queries against entities with specified component signatures with minimal overhead.
- **Entity Lifecycle & Recycling**: Automated ID recycling via internal free-lists preventing integer overflow during high-churn gameplay.
- **No External Dependencies**: Standard C++17 library only.

---

## Architectural Layout

```text
ecs/
├── CMakeLists.txt
├── DESIGN.md                  # Deep architectural design note & DoD analysis
├── README.md
├── include/
│   └── ecs/
│       ├── component_pool.hpp # Cache-friendly Sparse Set implementation
│       ├── components.hpp     # Standard POD components (Position, Velocity, Health, etc.)
│       ├── entity.hpp         # 32-bit EntityID definitions
│       ├── registry.hpp       # Central coordinator and multi-component view queries
│       └── systems.hpp        # Stateless systems (MovementSystem, CombatSystem)
├── src/
│   └── main_ecs.cpp           # Standalone high-scale benchmarking CLI
└── tests/
    └── test_ecs.cpp           # 6 automated unit and stress test cases
```

---

## Building and Running

### Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Run the Benchmark CLI Demo

```bash
./ecs_demo 50000
```

#### Sample Demo Output:
```text
==========================================================================
          game-dev-labs // Entity Component System (ECS) Demo            
     Data-Oriented Sparse Set Architecture & High-Scale Iteration         
==========================================================================

[*] Spawning 50000 game entities with Position & Velocity components...
[+] Successfully spawned 50000 entities in 4.18 ms (11961722 entities/sec)

--- Component Pool Memory Layout ---
  Position Components : 50000 contiguous records
  Velocity Components : 50000 contiguous records
  Health Components   : 25000 contiguous records
  Renderable Items    : 10000 contiguous records

[*] Executing MovementSystem over 100 simulation ticks (dt = 0.0167s)...
[+] Completed 5000000 component updates in 89.20 ms.
    Avg Frame Time : 0.892 ms (1121 simulated FPS equivalent)
    Throughput     : 56.1 Million entity updates/sec

[*] Simulating combat damage on first 5,000 entities...
[+] Dead entities harvested: 2500. Active entities remaining: 47500
[+] Recreated entities received recycled IDs: 4998, 4996

==========================================================================
ECS demonstration and memory stress test concluded successfully.
==========================================================================
```

### Run Automated Unit Tests

```bash
./ecs_tests
```

```text
=== Running ecs test suite ===
[+] Running test_entity_creation_destruction_and_recycling...
    PASSED
[+] Running test_component_emplace_get_and_has...
    PASSED
[+] Running test_sparse_set_swap_and_pop_integrity...
    PASSED
[+] Running test_multi_component_view_filtering...
    PASSED
[+] Running test_system_movement_and_combat...
    PASSED
[+] Running test_high_volume_stress...
    PASSED
=== All 6 ecs tests passed successfully! ===
```

---

## Design Trade-offs & Limitations

1. **Memory for Sparse Arrays**:
   - The Sparse Set allocates an array of dense indices up to the maximum active entity ID. While memory overhead is modest (4–8 bytes per entity slot), sparse entity allocations with large numeric gaps consume more memory than dense chunked archetypes.
2. **Component Pool Registration**:
   - Component pools are instantiated lazily at runtime via `typeid`. While dynamic and clean, component registration cannot be statically verified at compile-time without template metaprogramming macros.

---

## Roadmap

- [x] Sparse Set contiguous storage with swap-and-pop removal.
- [x] $O(1)$ component addition, lookup, and deletion.
- [x] Multi-component view filter queries.
- [x] Entity ID recycling and lifecycle tracking.
- [ ] **Component Event Signals**: Reactive callbacks (`on_add`, `on_remove`, `on_update`) for event-driven system triggers.
- [ ] **Multi-Threaded System Dispatcher**: Task graph scheduler allowing independent systems (e.g., Movement and Audio) to run concurrently across worker threads.
- [ ] **Serialization & Snapshots**: Automated component reflection for binary state saving and network synchronization.
- [ ] **Archetype Chunk Backend**: Alternate chunk-based storage option for homogenous mass entity counts (>500k entities).

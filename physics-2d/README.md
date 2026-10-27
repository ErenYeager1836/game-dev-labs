# 2D Physics & SAT Collision Resolution Engine

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../LICENSE)

A modular, real-time **2D Rigid Body Physics Engine** in modern C++17 implementing the **Separating Axis Theorem (SAT)** for narrowphase collision detection and impulse-based dynamics with restitution and Coulomb friction.

---

## Key Features

- **Separating Axis Theorem (SAT)**: High-accuracy collision detection and Minimum Translation Vector (MTV) penetration extraction for convex polygons and circles.
- **Impulse Dynamics**: Momentum-conserving velocity resolution supporting tunable restitution ($e \in [0.0, 1.0]$) and surface friction ($\mu$).
- **Positional Projection**: Anti-penetration slop stabilization preventing objects from sinking into resting surfaces.
- **Broadphase AABB Filtering**: Fast bounding box rejection pruning distant object pairs prior to narrowphase SAT projection.
- **Static & Dynamic Bodies**: Support for infinite-mass static boundaries (floors, walls, ramps) and dynamic active bodies.
- **Header-Only / Clean Architecture**: Self-contained C++17 with zero third-party dependencies.

---

## Architectural Layout

```text
physics-2d/
├── CMakeLists.txt
├── DESIGN.md               # Mathematical derivations (SAT, Impulse, Coulomb friction)
├── README.md
├── include/
│   └── physics2d/
│       ├── aabb.hpp        # Axis-Aligned Bounding Box broadphase
│       ├── body.hpp        # RigidBody dynamics and kinematic properties
│       ├── collision.hpp   # SAT collision detection routines
│       ├── manifold.hpp    # Contact manifold (normal, depth, contact point)
│       ├── shapes.hpp      # Circle and convex Polygon primitives
│       ├── vec2.hpp        # 2D vector mathematics (dot, cross, reflection, rotation)
│       └── world.hpp       # PhysicsWorld orchestrator & solver loop
├── src/
│   └── main_physics.cpp    # CLI bouncing ball & telemetry simulation
└── tests/
    └── test_physics.cpp    # 7 automated unit & momentum conservation tests
```

---

## Building and Running

### Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Run the Physics Simulation Demo

```bash
./physics_demo
```

#### Sample Demo Output:
```text
==========================================================================
    game-dev-labs // 2D Physics & SAT Collision Resolution Engine         
       Separating Axis Theorem (SAT) & Impulse-Based Dynamics             
==========================================================================

[*] Simulation Setup:
    Gravity       : (0, -9.81) m/s^2
    Floor         : Static AABB at y = -0.5m (mass = INF, restitution = 0.8)
    Ball          : Circle (radius = 0.5m, mass = 1.5kg, initial y = 8.0m)

--- Telemetry Output (Time, Position, Velocity, Kinetic Energy) ---
Step    Time (s)  Pos Y (m)     Vel Y (m/s)     Energy (J)    ASCII Visual Altitude
--------------------------------------------------------------------------
0       0.00      8.000         0.000           0.00          ======================== (O)
3       0.06      7.947         -1.766          2.34          ======================= (O)
6       0.12      7.794         -3.532          9.35          ======================= (O)
9       0.18      7.535         -5.297          21.05         ====================== (O)
12      0.24      7.170         -7.063          37.42         ===================== (O)
15      0.30      6.699         -8.829          58.46         ==================== (O)
...
36      0.72      0.102         -12.012         108.22        = (O)
39      0.78      0.628         9.850           72.77         == (O)  <-- BOUNCE 1!
42      0.84      1.162         8.084           49.01         === (O)
...

[+] Simulation finished. Detected 2 successful restitution bounces against static ground.
==========================================================================
```

### Run Automated Unit Tests

```bash
./physics_tests
```

```text
=== Running physics-2d test suite ===
[+] Running test_vec2_operations...
    PASSED
[+] Running test_aabb_intersections...
    PASSED
[+] Running test_circle_circle_collision...
    PASSED
[+] Running test_polygon_polygon_sat...
    PASSED
[+] Running test_circle_polygon_sat...
    PASSED
[+] Running test_momentum_conservation_elastic_collision...
    PASSED
[+] Running test_static_body_infinite_mass_bounce...
    PASSED
=== All 7 physics-2d tests passed successfully! ===
```

---

## Design Trade-offs & Limitations

1. **Convex Geometry Only**:
   - The Separating Axis Theorem mathematically applies only to convex shapes. Concave polygons must be decomposed into convex sub-polygons (e.g. via Ear Clipping or Hertel-Mehlhorn).
2. **Rotational Inertia / Torque**:
   - The current impulse solver models linear momentum and surface friction. Angular momentum ($I \alpha = \tau$) is currently non-coupled to contact tangents.
3. **Continuous Collision Detection (CCD)**:
   - Uses discrete time stepping; extremely high-velocity bullets may experience tunneling if displacement per frame exceeds shape thickness.

---

## Roadmap

- [x] Vector2 mathematics with dot, cross, reflection, and rotation.
- [x] Separating Axis Theorem (SAT) for convex polygons and circles.
- [x] Impulse velocity solver with restitution and Coulomb friction.
- [x] Anti-penetration slop projection.
- [ ] **Rotational Dynamics & Moment of Inertia**: Coupling torque and angular velocity to contact points ($J_t \times r$).
- [ ] **Spatial Hash / Dynamic BVH Tree**: Replacing $O(N^2)$ pairwise checks with a dynamic AABB bounding volume hierarchy.
- [ ] **Continuous Collision Detection (CCD)**: Raycast / swept circle continuous time-of-impact (TOI) solver to eliminate tunneling.
- [ ] **Constraint Solvers**: Distance joints, revolute pin joints, and spring dampers.

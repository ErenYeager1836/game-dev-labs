# Architectural Design Note: 2D Physics & SAT Collision Engine

## 1. Executive Summary & Physics Architecture

This 2D physics engine provides continuous physical simulation and narrowphase collision resolution for arbitrary convex polygons and circles.

Core mathematical systems:
1. **Separating Axis Theorem (SAT)**: Accurate narrowphase collision testing and Minimum Translation Vector (MTV) extraction for convex 2D geometry.
2. **Impulse-Based Velocity Solver**: Linear momentum conservation with configurable restitution (elasticity $e$) and Coulomb friction ($\mu$).
3. **Baumgarte / Projection Slop Stabilization**: Positional correction preventing penetration accumulation and simulation sinking without introducing artificial energy.
4. **Broadphase AABB Pruning**: Axis-Aligned Bounding Box early-rejection reducing narrowphase checks from $O(N^2)$ to $O(K)$.

---

## 2. Mathematical Foundations

### 2.1 The Separating Axis Theorem (SAT)
For any two convex shapes $A$ and $B$, if there exists a 1D axis $L$ along which the orthogonal projections of $A$ and $B$ do not overlap, then $A$ and $B$ are **disjoint** (not colliding).

```text
       Axis L: --------------[----A----]------[----B----]------------->
                             minA     maxA    minB     maxB
                                      GAP DETECTED -> NO COLLISION!
```

#### Candidate Axes:
- **Polygon vs. Polygon**: The unit normal vectors of all edges of Polygon $A$ and Polygon $B$.
  $$\vec{n}_i = \frac{(v_{i+1}.y - v_i.y, -(v_{i+1}.x - v_i.x))}{\|\vec{n}_i\|}$$
- **Circle vs. Polygon**: All edge normals of the polygon, plus the normalized axis from the circle's center to the polygon's closest vertex.

#### Overlap Calculation:
When intervals overlap along axis $\vec{L}$:
$$\text{overlap} = \min(\max_A, \max_B) - \max(\min_A, \min_B)$$
The axis with the **minimum overlap** is chosen as the **contact normal** $\vec{n}$, and the minimum overlap distance is the **penetration depth** $d$.

---

### 2.2 Impulse-Based Collision Resolution

When two bodies collide at contact point with normal $\vec{n}$ pointing from $A$ to $B$:
- Relative contact velocity:
  $$\vec{v}_{rel} = \vec{v}_B - \vec{v}_A$$
- Relative normal speed:
  $$v_n = \vec{v}_{rel} \cdot \vec{n}$$
  *(If $v_n > 0$, the objects are already moving away from each other; impulse is skipped).*

#### Normal Impulse Magnitude ($J$):
From Newton's restitution law ($v_n^+ = -e v_n^-$):
$$J = -\frac{(1 + e) v_n}{m_A^{-1} + m_B^{-1}}$$
where $e = \min(e_A, e_B)$ is the coefficient of restitution, and $m^{-1}$ is inverse mass ($0$ for static objects).

#### Velocity Updates:
$$\vec{v}_A \leftarrow \vec{v}_A - J \vec{n} m_A^{-1}$$
$$\vec{v}_B \leftarrow \vec{v}_B + J \vec{n} m_B^{-1}$$

#### Tangent Friction Impulse ($J_t$):
$$\vec{t} = \frac{\vec{v}_{rel} - (\vec{v}_{rel} \cdot \vec{n}) \vec{n}}{\|\vec{v}_{rel} - (\vec{v}_{rel} \cdot \vec{n}) \vec{n}\|}$$
$$J_t = -\frac{\vec{v}_{rel} \cdot \vec{t}}{m_A^{-1} + m_B^{-1}}$$
Clamped by Coulomb's Law of Friction ($|J_t| \le \mu |J|$):
$$J_t \leftarrow \text{clamp}(J_t, -\mu |J|, \mu |J|)$$

---

### 2.3 Positional Correction (Penetration Slop)

Due to discrete time stepping ($\Delta t$), bodies penetrate slightly before collisions are registered. Resolving only velocities causes resting objects to gradually sink due to constant gravity.

To eliminate this artifact without imparting kinetic energy, a small fraction (40%) of the penetration exceeding a small allowance ($\text{slop} = 0.01\text{m}$) is projected directly into position:
$$\vec{C} = \vec{n} \cdot \frac{\max(d - \text{slop}, 0)}{m_A^{-1} + m_B^{-1}} \times 0.4$$
$$\vec{x}_A \leftarrow \vec{x}_A - \vec{C} m_A^{-1}$$
$$\vec{x}_B \leftarrow \vec{x}_B + \vec{C} m_B^{-1}$$

---

## 3. Class Architecture

```text
PhysicsWorld
├── std::vector<RigidBody>
├── Vec2 gravity
└── step(dt)
     ├── 1. Integrate Forces (v += (g + F/m) * dt)
     ├── 2. Broadphase AABB Overlap Tests
     ├── 3. CollisionDetector (SAT Narrowphase)
     │       ├── test_circle_circle()
     │       ├── test_polygon_polygon()
     │       └── test_circle_polygon()
     ├── 4. Positional Projection (Anti-sink slop)
     ├── 5. Impulse Velocity Resolution (Normal + Friction)
     └── 6. Integrate Positions (x += v * dt)
```

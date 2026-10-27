# Architectural Design Note: Procedural Level Generation & A* Pathfinding

## 1. Executive Summary & Design Goals

This engine provides algorithmic solutions for two foundational problems in game development:
1. **Procedural Level Generation (PCG)**: Algorithmic creation of structured, 100% interconnected dungeons (Binary Space Partitioning) and natural organic environments (Cellular Automata).
2. **Optimal Pathfinding**: Shortest-path navigation using the $A^*$ search algorithm with admissible heuristics, priority queue pruning, and corner-cutting prevention.

---

## 2. Pathfinding: $A^*$ Search Theory & Optimizations

$A^*$ evaluates candidates using the evaluation function:
$$f(n) = g(n) + h(n)$$
- $g(n)$: Exact accumulated cost from the start node to current node $n$.
- $h(n)$: Estimated heuristic cost from current node $n$ to the goal.
- $f(n)$: Total estimated path cost through node $n$.

### 2.1 Admissibility and Consistency
For $A^*$ to guarantee the **optimal shortest path**:
1. **Admissibility**: The heuristic $h(n)$ must never overestimate the true minimum cost to the goal ($h(n) \le h^*(n)$).
2. **Consistency (Monotonicity)**: For every node $n$ and neighbor $n'$:
   $$h(n) \le c(n, n') + h(n')$$
   Consistency ensures that when a node is removed from the priority queue (expanded), the discovered path to it is guaranteed to be optimal; nodes never need to be re-opened.

### 2.2 Grid Heuristics
- **4-Directional Movement (Cardinal)**:
  $$h(n) = |x_n - x_{goal}| + |y_n - y_{goal}| \quad \text{(Manhattan Distance)}$$
- **8-Directional Movement (With Diagonals)**:
  $$h(n) = \max(\Delta x, \Delta y) + (\sqrt{2} - 1) \min(\Delta x, \Delta y) \quad \text{(Octile Distance)}$$

### 2.3 Tie-Breaking Optimization
When multiple paths have identical $f(n)$ costs, standard priority queues expand large diamond-shaped plateaus. This engine implements **tie-breaking on lower $h(n)$**, prioritizing nodes that are geometrically closer to the goal and significantly pruning search space exploration.

---

## 3. Procedural Content Generation (PCG)

### 3.1 Binary Space Partitioning (BSP) Dungeon Generation
```text
           [Root: Entire Map Area]
                   |
        +----------+----------+
        | (H-Split)           |
     [Leaf A]              [Leaf B]
        | (V-Split)           |
   +----+----+           +----+----+
[Leaf 1]   [Leaf 2]   [Leaf 3]   [Leaf 4]
(Room 1)   (Room 2)   (Room 3)   (Room 4)
```

1. **Recursive Partitioning**: The boundary is recursively split horizontally or vertically with randomized position offsets until partitions reach minimum room constraints.
2. **Room Carving**: Each leaf partition carves an interior room with randomized dimensions padded away from partition walls.
3. **Corridor Connectivity**: Adjacent rooms are connected via L-shaped orthogonal corridors:
   $$\text{Room}_i \xrightarrow{\text{Horizontal Corridor}} (x_{i+1}, y_i) \xrightarrow{\text{Vertical Corridor}} \text{Room}_{i+1}$$
   *Guarantee*: Every generated dungeon is provably $100\%$ walkable and interconnected without dead-end islands.

### 3.2 Cellular Automata Cave Smoothing
Simulates Conway-style cellular growth to generate natural limestone caves and organic caverns:
1. **Randomized Seeding**: Grid initialized with a $45\%$ wall density.
2. **Neighborhood Evaluation**: Counts solid wall neighbors in the $3 \times 3$ Moore neighborhood.
3. **4-5 Transition Rule**:
   $$\text{State}_{t+1} = \begin{cases} \text{Wall} & \text{if } \text{neighbors} \ge 5 \lor \text{neighbors} = 0 \\ \text{Floor} & \text{otherwise} \end{cases}$$
Iterating 4–5 generations produces natural, aesthetic cave layouts with smoothed walls.

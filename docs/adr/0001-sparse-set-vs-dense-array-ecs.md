# ADR 0001: Sparse Set vs. Contiguous Dense Arrays in Entity Component System

**Status:** Accepted  
**Date:** 2026-10-28  
**Context:** Core Engine Architecture (`ecs`)  
**Deciders:** Nithishvelan M  

---

## 1. Context and Problem Statement

In real-time game simulations, the rendering and physics loop executes 60 to 144 times per second (16.6ms to 6.94ms per frame). A typical scene contains tens of thousands of dynamic entities updating transform coordinates, spatial hierarchies, and physics components.

Traditional Object-Oriented Programming (OOP) architectures structure entities as polymorphic classes (`class Monster : public GameObject`) stored via pointers (`std::vector<std::unique_ptr<GameObject>>`). This pattern incurs:
1. **Pervasive Pointer Chasing:** Scattered heap allocations cause catastrophic L1/L2 data cache evictions.
2. **Cache-Line Underutilization:** Loading a 64-byte CPU cache line to access an 8-byte `Position` brings in unused variables (sound pointers, mesh handles, AI state).
3. **Dynamic Dispatch Latency:** Virtual function calls (`virtual void update()`) disrupt instruction pipeline branch predictors.

To maximize throughput, the ECS architecture must satisfy three opposing operational constraints:
* **$O(1)$ Direct Lookup:** Querying whether Entity $E$ possesses Component $C$ must be immediate.
* **$O(1)$ Insertion and Deletion:** Spawning or destroying entities must not require contiguous memory shifts.
* **100% Dense Cache Locality:** Hot simulation systems must traverse components as sequential, contiguous arrays with zero memory gaps or null checks.

---

## 2. Decision: Sparse Set Architecture

We chose the **Sparse Set** data structure for component storage across all component pools in `ecs`.

A Sparse Set consists of two coupled arrays:
1. **`sparse` Array:** Indexed directly by `EntityID`. Stores the index of the entity's data in the dense array, or a sentinel value (`INVALID_INDEX`) if the entity lacks the component.
2. **`dense` Array:** A tightly packed, contiguous `std::vector<EntityID>` storing the entity IDs that currently hold the component.
3. **`components` Array:** A parallel, tightly packed, contiguous `std::vector<Component>` storing the plain-old-data (POD) structs.

```
Entity ID: 4
Sparse Index Array:
Index:   0    1    2    3    4    5
Value: [ -1 | -1 |  0 | -1 |  1 | -1 ]
                    |         |
                    +----+    +-------+
                         v            v
Dense Entities:        [ 2    |       4    ]   (Packed, contiguous)
Dense Components:      [ P_2  |       P_4  ]   (Packed, contiguous, 64-byte cache line friendly)
```

### Deletion Mechanics: Swap-and-Pop
To delete an entity in $O(1)$ time without introducing fragmentation into the dense array:
1. Identify the dense index of the target entity from `sparse[entity]`.
2. Swap the target entity's component with the last element in the dense array.
3. Update the sparse index of the moved last element to point to the swapped position.
4. Call `pop_back()` on the dense vectors.

---

## 3. Alternative Considered: Pure Dense Array with Bitsets

### Architecture:
* Allocate a flat `std::vector<Component>` where index $i$ corresponds directly to Entity $i$.
* Maintain a bitset (`std::bitset<MAX_ENTITIES>`) tracking component presence.

### Trade-offs & Reasons for Rejection:
* **Memory Bloat:** If Entity 1 and Entity 50,000 are the only entities with a `Health` component, the system allocates memory for 50,000 components, resulting in > 99.9% wasted memory.
* **Iterative Inefficiency:** Hot loops must inspect bitset flags on every iteration. Skipping missing entities causes CPU branch mispredictions and forces the cache to load empty lines.
* **Sparse Set Superiority:** Sparse sets consume memory proportional only to the *active* component count in the dense arrays, while sparse page allocations can be lazily paged.

---

## 4. Consequences & Performance Profile

### Positive:
* **Zero Pointer Overhead:** Components are pure Plain-Old-Data structs (`struct Position { float x, y; };`) without vtable pointers.
* **L1 Cache Utilization:** Sequential systems (e.g., `MovementSystem`) achieve near 100% prefetcher hit rates because components are loaded sequentially into cache lines.
* **Microbenchmarking Validation:** In automated 50,000-entity stress benchmarks (`main_ecs.cpp`), iteration and update across 50,000 entities completes in `< 0.82ms` on modern x86-64 hardware.

### Negative / Trade-offs:
* **Sparse Memory Overhead:** The sparse array requires an integer entry per potential Entity ID ($4 \text{ bytes} \times \text{Max Entities}$). For 100,000 max entities, this is $\approx 400\text{ KB}$ per component type, which is negligible on modern host architectures.

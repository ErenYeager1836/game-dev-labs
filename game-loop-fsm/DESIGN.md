# Architectural Design Note: Fixed-Timestep Game Loop & Hierarchical State Machine (HFSM)

## 1. Executive Summary & Core Problems

Real-time video games face two persistent challenges:
1. **Decoupling Simulation from Rendering**: Ensuring physics behaves identically regardless of whether the game is running on a 30 FPS console, a 144 Hz gaming monitor, or suffering from a temporary frame stutter.
2. **State Management Complexity**: Managing character input states (Idle, Walk, Jump, Fall, Attack, Stun) without combinatorial boolean flag explosions (`is_jumping`, `is_attacking`, `can_move`, `was_grounded`).

This module solves both problems with mathematical rigor:
- **Glenn Fiedler's "Fix Your Timestep!"** accumulator algorithm.
- **Hierarchical Finite State Machine (HFSM)** with explicit transition lifecycles (`on_enter`, `on_update`, `on_exit`).
- **Decoupled Pub/Sub Event Bus** for cross-system event dispatching.

---

## 2. The Fixed-Timestep Accumulator Algorithm

### 2.1 The Problem with Variable $\Delta t$ in Physics
In naive game engines:
```cpp
// DANGEROUS NAIVE LOOP
while (running) {
    float dt = timer.get_delta_time();
    physics.update(dt); // Physics step size depends directly on rendering frame rate!
    renderer.render();
}
```
If a frame drops from 60 FPS ($dt \approx 16.6\text{ms}$) to 10 FPS ($dt = 100\text{ms}$), numerical integration errors explode:
$$v_{t+1} = v_t + a \Delta t$$
$$x_{t+1} = x_t + v \Delta t$$
Springs explode, collision detection tunnels through walls, and deterministic multiplayer replays diverge.

### 2.2 The Accumulator Solution
```text
Real Time Elapsed:  |------------------ Frame dt (e.g. 45 ms) -------------------|
Accumulator:        [ 16.6 ms (Tick 1) ][ 16.6 ms (Tick 2) ][ 11.8 ms (Remainder) ]
Simulation Steps:   =====> TICK 1 ======> TICK 2 ======> (Wait for next frame)
Render State:       Interpolate(Prev_State, Curr_State, alpha = 11.8 / 16.6 = 0.71)
```

1. **Spiral of Death Protection**: If a frame takes unusually long (e.g., 500 ms due to an OS context switch), processing dozens of physics steps in a row causes the next frame to take even longer, freezing the engine. Clamping $\Delta t$ to `max_frame_time` (e.g. 250 ms) bounds the maximum simulation steps per frame.
2. **State Interpolation**: When the remainder in the accumulator leaves physics partway between two fixed steps:
   $$\alpha = \frac{\text{accumulator}}{\text{fixed\_dt}}$$
   The renderer displays:
   $$\text{Render Position} = \text{Position}_{prev} \cdot (1 - \alpha) + \text{Position}_{curr} \cdot \alpha$$
   This produces perfectly smooth visuals at 144Hz, 240Hz, or higher, even though physics ticks at a stable 60Hz.

---

## 3. Hierarchical Finite State Machine (HFSM)

### 3.1 State Diagram
```text
           +------------------+
           |       Idle       |<-----------------+
           +--------+---------+                  |
             |            |                      |
   move_left/right      jump                     |
             v            v                      |
      +------------+  +------------+             |
      |    Walk    |  |    Jump    |             |
      +-----+------+  +-----+------+             |
            |               |                    |
          jump           vy <= 0                 |
            |               v                    |
            |         +------------+             |
            +-------->|    Fall    |             |
                      +-----+------+             |
                            |                    |
                       y <= 0 (Land)             |
                            |                    |
                            +--------------------+
                                     ^
                                     | (after 250ms)
       (any state) --attack--> +------------+
                               |   Attack   |
                               +------------+
```

### 3.2 Benefits over Boolean Flags
- **Combinatorial Explosion Prevented**: Instead of managing $2^N$ possible combinations of booleans, only valid transitions are permissible.
- **Encapsulated Lifecycle**: Entering `Attack` locks movement duration; exiting `Jump` resets velocity.
- **Event-Driven**: Transitions trigger decoupled notifications (`PlayerJumpedEvent`, `StateChangedEvent`) through the `EventBus` without coupling physics to audio or animation controllers.

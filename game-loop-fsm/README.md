# Fixed-Timestep Game Loop & Hierarchical State Machine (HFSM)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../LICENSE)

A modular **Game Loop & Hierarchical Finite State Machine (HFSM)** engine in modern C++17 implementing the industry-standard **"Fix Your Timestep!"** accumulator pattern, render interpolation factor calculation ($\alpha$), decoupled **Pub/Sub Event Bus**, and a resilient **Character Controller State Machine**.

---

## Key Features

- **Fixed-Timestep Accumulator**: Completely decouples deterministic physics simulation (e.g. fixed 60Hz) from variable graphics render refresh rates (30Hz, 60Hz, 144Hz+).
- **Spiral-of-Death Protection**: Clamps frame delta times against temporary CPU stalls, preventing cascading freeze loops.
- **Render State Interpolation**: Computes sub-frame alpha blending factors ($\alpha \in [0.0, 1.0)$) to eliminate physics visual stutter during high-refresh rendering.
- **Hierarchical Finite State Machine (HFSM)**:
  - Clean separation of states (`Idle`, `Walk`, `Jump`, `Fall`, `Attack`).
  - Strict lifecycle methods: `on_enter`, `on_update`, `on_exit`, and `handle_input`.
  - Rejection of illegal state transitions (e.g. jumping while airborne or moving while locked in attack animation).
- **Type-Safe Decoupled Event Bus**: Zero-overhead publisher/subscriber dispatcher decoupling gameplay actions from audio, particle, and UI systems.

---

## Architectural Layout

```text
game-loop-fsm/
├── CMakeLists.txt
├── DESIGN.md               # Loop mathematics, integrator divergence, and FSM theory
├── README.md
├── include/
│   └── gameloop/
│       ├── event_bus.hpp   # Type-safe EventBus (subscribe/publish)
│       ├── state_machine.hpp # State, StateMachine, and Character Controller states
│       └── timestep.hpp    # TimestepManager accumulator & lerp helper
├── src/
│   └── main_loop.cpp       # CLI simulation of loop, inputs, and state transitions
└── tests/
    └── test_game_loop.cpp  # 7 automated unit & timing validation test cases
```

---

## Building and Running

### Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Run the Game Loop & FSM Demo

```bash
./gameloop_demo
```

#### Sample Demo Output:
```text
==========================================================================
  game-dev-labs // Fixed-Timestep Loop & Hierarchical State Machine (HFSM)
       Decoupled Physics / Render Timing & Character FSM Architecture     
==========================================================================

[*] Simulation Configuration:
    Fixed Physics Tick : 60 Hz (0.0167 s)
    Initial State      : Idle

--- Telemetry Output (Time, Tick, State, Position X/Y, Alpha, Event Log) ---
Time (s)  Ticks   State       Pos (X, Y)      Alpha     Latest Event
--------------------------------------------------------------------------
0.095     5       Idle        (0.00, 0.00)    0.70      StateTransition: None -> Idle
0.190     11      Walk        (0.33, 0.00)    0.40      StateTransition: Idle -> Walk
0.285     17      Jump        (0.66, 0.95)    0.10      Event: Player 1 JUMPED (vy = 8 m/s)
0.380     22      Jump        (1.00, 1.55)    0.80      Event: Player 1 JUMPED (vy = 8 m/s)
0.475     28      Fall        (1.33, 1.48)    0.50      StateTransition: Jump -> Fall
0.570     34      Fall        (1.66, 0.72)    0.20      StateTransition: Jump -> Fall
0.665     39      Walk        (2.00, 0.00)    0.90      Event: Player 1 LANDED (impact = 0)
0.760     45      Walk        (2.40, 0.00)    0.60      Event: Player 1 LANDED (impact = 0)
0.855     51      Walk        (2.80, 0.00)    0.30      Event: Player 1 LANDED (impact = 0)
0.950     57      Attack      (2.80, 0.00)    0.00      Event: Player 1 ATTACK (Slash, 35 dmg)
1.045     62      Idle        (2.80, 0.00)    0.70      StateTransition: Attack -> Idle

[+] Complete State Transition History (6 transitions):
    Idle -> Walk -> Jump -> Fall -> Walk -> Attack -> Idle

==========================================================================
Fixed-timestep and FSM demonstration concluded successfully.
==========================================================================
```

### Run Automated Unit Tests

```bash
./gameloop_tests
```

```text
=== Running game-loop-fsm test suite ===
[+] Running test_timestep_accumulator_and_fixed_ticks...
    PASSED
[+] Running test_timestep_spiral_of_death_clamp...
    PASSED
[+] Running test_fsm_idle_to_walk_transitions...
    PASSED
[+] Running test_fsm_jump_fall_land_progression...
    PASSED
[+] Running test_fsm_attack_animation_timer...
    PASSED
[+] Running test_event_bus_dispatching...
    PASSED
[+] Running test_lerp_interpolation...
    PASSED
=== All 7 game-loop-fsm tests passed successfully! ===
```

---

## Design Trade-offs & Limitations

1. **Interpolation Latency**:
   - Linear interpolation between the previous and current physics state displays rendering lagged by exactly one simulation step ($1/60\text{ s} \approx 16.6\text{ms}$). For ultra-competitive twitch shooters, state extrapolation is sometimes favored despite risk of overshooting.
2. **Deterministic Replays across Architectures**:
   - While fixed timesteps guarantee determinism on the same compiler and machine, cross-platform multiplayer lockstep determinism (e.g. x86 vs ARM64) requires fixed-point mathematics (`Q16.16`) rather than standard floating-point `float` operations due to differences in FPU rounding modes.

---

## Roadmap

- [x] Fixed-timestep accumulator loop with spiral-of-death clamp.
- [x] Render interpolation blending factor ($\alpha$).
- [x] Decoupled pub/sub `EventBus`.
- [x] Hierarchical character state machine with lifecycle callbacks.
- [ ] **State Machine Stack (Push/Pop Substates)**: Adding state stacking (e.g. pushing a `PauseMenuState` or `InventoryState` over the active gameplay state without destroying it).
- [ ] **Fixed-Point Determinism Module**: Fixed-point integer math (`fixed32`) for 100% cross-architecture lockstep networking.
- [ ] **Behavior Tree Engine**: Hierarchical Selector/Sequence composite nodes with Blackboards for advanced non-player character (NPC) artificial intelligence.
- [ ] **Frame Budget Profiler**: Built-in microsecond instrumentation tracking CPU tick budget utilization and detecting physics frame drops.

#include "gameloop/event_bus.hpp"
#include "gameloop/state_machine.hpp"
#include "gameloop/timestep.hpp"

#include <iomanip>
#include <iostream>
#include <vector>

using namespace gamedev::gameloop;

int main() {
    std::cout << "==========================================================================\n";
    std::cout << "  game-dev-labs // Fixed-Timestep Loop & Hierarchical State Machine (HFSM)\n";
    std::cout << "       Decoupled Physics / Render Timing & Character FSM Architecture     \n";
    std::cout << "==========================================================================\n\n";

    EventBus event_bus;
    std::vector<std::string> log_events;

    // Subscribe to gameplay and state events
    event_bus.subscribe<StateChangedEvent>([&](const StateChangedEvent& e) {
        log_events.push_back("StateTransition: " + e.from_state + " -> " + e.to_state);
    });

    event_bus.subscribe<PlayerJumpedEvent>([&](const PlayerJumpedEvent& e) {
        log_events.push_back("Event: Player " + std::to_string(e.player_id) + " JUMPED (vy = " +
                             std::to_string(static_cast<int>(e.vertical_velocity)) + " m/s)");
    });

    event_bus.subscribe<PlayerLandedEvent>([&](const PlayerLandedEvent& e) {
        log_events.push_back("Event: Player " + std::to_string(e.player_id) + " LANDED (impact = " +
                             std::to_string(static_cast<int>(e.impact_speed)) + ")");
    });

    event_bus.subscribe<AttackExecutedEvent>([&](const AttackExecutedEvent& e) {
        log_events.push_back("Event: Player " + std::to_string(e.player_id) + " ATTACK (" +
                             e.attack_type + ", " + std::to_string(e.base_damage) + " dmg)");
    });

    // Initialize State Machine with EventBus
    StateMachine fsm(&event_bus);
    fsm.change_state(std::make_unique<IdleState>());

    // Timestep: 60Hz fixed physics updates
    TimestepManager timestep(1.0 / 60.0, 0.25);

    std::cout << "[*] Simulation Configuration:\n";
    std::cout << "    Fixed Physics Tick : 60 Hz (" << std::fixed << std::setprecision(4)
              << timestep.fixed_dt() << " s)\n";
    std::cout << "    Initial State      : " << fsm.current_state_name() << "\n\n";

    std::cout << "--- Telemetry Output (Time, Tick, State, Position X/Y, Alpha, Event Log) ---\n";
    std::cout << std::left << std::setw(10) << "Time (s)"
              << std::setw(8)  << "Ticks"
              << std::setw(12) << "State"
              << std::setw(16) << "Pos (X, Y)"
              << std::setw(10) << "Alpha"
              << "Latest Event\n";
    std::cout << "--------------------------------------------------------------------------\n";

    // Simulate 75 frames of gameplay with variable delta time (~16.6ms to 33.3ms)
    double sim_time = 0.0;
    for (int frame = 0; frame < 75; ++frame) {
        // Variable frame render delta time (simulating rendering fluctuation)
        double frame_dt = (frame % 3 == 0) ? 0.025 : 0.015;
        sim_time += frame_dt;

        // Scripted user input stream at key milestones
        if (frame == 5) {
            fsm.handle_input("move_right");
        } else if (frame == 18) {
            fsm.handle_input("jump");
        } else if (frame == 55) {
            fsm.handle_input("attack");
        }

        // Process fixed timestep physics updates
        timestep.process_frame(frame_dt, [&](float dt) {
            fsm.update(dt);
        });

        // Print telemetry every 5 frames
        if (frame % 5 == 0) {
            std::string pos_str = "(" + std::to_string(fsm.context.x).substr(0, 4) + ", " +
                                  std::to_string(fsm.context.y).substr(0, 4) + ")";
            std::string latest_event = log_events.empty() ? "-" : log_events.back();

            std::cout << std::left << std::setw(10) << std::fixed << std::setprecision(3) << sim_time
                      << std::setw(8)  << timestep.total_ticks()
                      << std::setw(12) << fsm.current_state_name()
                      << std::setw(16) << pos_str
                      << std::setw(10) << std::setprecision(2) << timestep.interpolation_alpha()
                      << latest_event << "\n";
        }
    }

    std::cout << "\n[+] Complete State Transition History (" << fsm.history().size() << " transitions):\n    ";
    for (size_t i = 0; i < fsm.history().size(); ++i) {
        std::cout << fsm.history()[i] << (i + 1 < fsm.history().size() ? " -> " : "\n\n");
    }

    std::cout << "==========================================================================\n";
    std::cout << "Fixed-timestep and FSM demonstration concluded successfully.\n";
    std::cout << "==========================================================================\n";

    return 0;
}

#include "gameloop/event_bus.hpp"
#include "gameloop/state_machine.hpp"
#include "gameloop/timestep.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace gamedev::gameloop;

void test_timestep_accumulator_and_fixed_ticks() {
    std::cout << "[+] Running test_timestep_accumulator_and_fixed_ticks...\n";
    // Fixed dt = 0.02s (50 Hz)
    TimestepManager tm(0.02, 0.25);

    int update_calls = 0;
    // Process 0.05s of frame time: should execute exactly 2 ticks (0.04s) and leave 0.01s accumulator
    tm.process_frame(0.05, [&](float dt) {
        assert(std::abs(dt - 0.02f) < 1e-5f);
        update_calls++;
    });

    assert(update_calls == 2);
    assert(tm.total_ticks() == 2);
    assert(std::abs(tm.accumulator() - 0.01) < 1e-5);
    // Alpha should be 0.01 / 0.02 = 0.5
    assert(std::abs(tm.interpolation_alpha() - 0.5f) < 1e-4f);

    std::cout << "    PASSED\n";
}

void test_timestep_spiral_of_death_clamp() {
    std::cout << "[+] Running test_timestep_spiral_of_death_clamp...\n";
    // Fixed dt = 0.01s, max clamp = 0.05s
    TimestepManager tm(0.01, 0.05);

    int update_calls = 0;
    // Feed massive 10.0s lag spike
    tm.process_frame(10.0, [&](float) {
        update_calls++;
    });

    // Should only execute 5 updates (0.05 / 0.01), preventing spiral of death
    assert(update_calls == 5);
    assert(tm.total_ticks() == 5);

    std::cout << "    PASSED\n";
}

void test_fsm_idle_to_walk_transitions() {
    std::cout << "[+] Running test_fsm_idle_to_walk_transitions...\n";
    StateMachine fsm;
    fsm.change_state(std::make_unique<IdleState>());

    assert(fsm.current_state_name() == "Idle");
    assert(fsm.context.vx == 0.0f);

    // Send move command
    fsm.handle_input("move_right");
    assert(fsm.current_state_name() == "Walk");
    assert(fsm.context.vx > 0.0f);

    // Update walk for 1 second
    fsm.update(1.0f);
    assert(fsm.context.x > 3.9f);

    // Stop command
    fsm.handle_input("stop");
    assert(fsm.current_state_name() == "Idle");
    assert(fsm.context.vx == 0.0f);

    std::cout << "    PASSED\n";
}

void test_fsm_jump_fall_land_progression() {
    std::cout << "[+] Running test_fsm_jump_fall_land_progression...\n";
    StateMachine fsm;
    fsm.change_state(std::make_unique<IdleState>());

    // Trigger jump
    fsm.handle_input("jump");
    assert(fsm.current_state_name() == "Jump");
    assert(!fsm.context.is_grounded);
    assert(fsm.context.vy > 0.0f);

    // Update through ascent to apex (vy drops below 0)
    while (fsm.current_state_name() == "Jump") {
        fsm.update(0.05f);
    }
    assert(fsm.current_state_name() == "Fall");

    // Update descent until landing on ground
    while (fsm.current_state_name() == "Fall") {
        fsm.update(0.05f);
    }

    // Must be back on ground
    assert(fsm.context.is_grounded);
    assert(fsm.context.y == 0.0f);
    assert(fsm.current_state_name() == "Idle");

    std::cout << "    PASSED\n";
}

void test_fsm_attack_animation_timer() {
    std::cout << "[+] Running test_fsm_attack_animation_timer...\n";
    StateMachine fsm;
    fsm.change_state(std::make_unique<IdleState>());

    fsm.handle_input("attack");
    assert(fsm.current_state_name() == "Attack");

    // Attack locks for 0.25s
    fsm.update(0.1f);
    assert(fsm.current_state_name() == "Attack");

    fsm.update(0.2f);
    // After 0.3s total, state returns to Idle
    assert(fsm.current_state_name() == "Idle");

    std::cout << "    PASSED\n";
}

void test_event_bus_dispatching() {
    std::cout << "[+] Running test_event_bus_dispatching...\n";
    EventBus bus;
    int received_count = 0;
    float received_vel = 0.0f;

    bus.subscribe<PlayerJumpedEvent>([&](const PlayerJumpedEvent& e) {
        received_count++;
        received_vel = e.vertical_velocity;
    });

    bus.publish(PlayerJumpedEvent{42, 9.5f});
    assert(received_count == 1);
    assert(received_vel == 9.5f);

    bus.publish(PlayerJumpedEvent{42, 12.0f});
    assert(received_count == 2);
    assert(received_vel == 12.0f);

    std::cout << "    PASSED\n";
}

void test_lerp_interpolation() {
    std::cout << "[+] Running test_lerp_interpolation...\n";
    assert(std::abs(lerp(0.0f, 100.0f, 0.0f) - 0.0f) < 1e-5f);
    assert(std::abs(lerp(0.0f, 100.0f, 0.5f) - 50.0f) < 1e-5f);
    assert(std::abs(lerp(0.0f, 100.0f, 1.0f) - 100.0f) < 1e-5f);
    assert(std::abs(lerp(20.0f, 60.0f, 0.25f) - 30.0f) < 1e-5f);

    std::cout << "    PASSED\n";
}

int main() {
    std::cout << "=== Running game-loop-fsm test suite ===\n";

    test_timestep_accumulator_and_fixed_ticks();
    test_timestep_spiral_of_death_clamp();
    test_fsm_idle_to_walk_transitions();
    test_fsm_jump_fall_land_progression();
    test_fsm_attack_animation_timer();
    test_event_bus_dispatching();
    test_lerp_interpolation();

    std::cout << "=== All 7 game-loop-fsm tests passed successfully! ===\n";
    return 0;
}

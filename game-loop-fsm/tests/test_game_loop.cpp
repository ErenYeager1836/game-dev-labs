#include "gameloop/behavior_tree.hpp"
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

class PauseMenuState : public State {
public:
    std::string name() const override { return "PauseMenu"; }
    void on_enter(StateMachine&) override { entered = true; }
    void on_exit(StateMachine&) override { exited = true; }
    bool entered = false;
    bool exited = false;
};

void test_state_machine_stack_push_pop() {
    std::cout << "[+] Running test_state_machine_stack_push_pop...\n";
    StateMachine fsm;

    fsm.change_state(std::make_unique<WalkState>());
    assert(fsm.current_state_name() == "Walk");
    assert(fsm.stack_depth() == 1);

    // Push pause menu overlay
    fsm.push_state(std::make_unique<PauseMenuState>());
    assert(fsm.current_state_name() == "PauseMenu");
    assert(fsm.stack_depth() == 2);

    // Pop pause menu, returning to Walk
    fsm.pop_state();
    assert(fsm.current_state_name() == "Walk");
    assert(fsm.stack_depth() == 1);

    std::cout << "    PASSED\n";
}

void test_behavior_tree_execution() {
    std::cout << "[+] Running test_behavior_tree_execution...\n";
    Blackboard bb;
    bb.set_float("target_distance", 15.0f);
    bb.set_string("current_action", "none");

    // Behavior Tree Structure:
    // Selector (Fallback)
    //   -> Sequence:
    //        -> Condition: target_distance <= 5.0f (In range)
    //        -> Action: set current_action = "melee_attack"
    //   -> Action: set current_action = "patrol"

    auto combat_seq = std::make_shared<SequenceNode>();
    combat_seq->add_child(std::make_shared<ConditionNode>([](Blackboard& b) {
        return b.get_float("target_distance") <= 5.0f;
    }));
    combat_seq->add_child(std::make_shared<ActionNode>([](Blackboard& b) {
        b.set_string("current_action", "melee_attack");
        return NodeStatus::Success;
    }));

    auto patrol_action = std::make_shared<ActionNode>([](Blackboard& b) {
        b.set_string("current_action", "patrol");
        return NodeStatus::Success;
    });

    SelectorNode root;
    root.add_child(combat_seq);
    root.add_child(patrol_action);

    // Tick 1: Target distance is 15.0f -> Condition fails, Sequence fails, Patrol executes
    NodeStatus status1 = root.tick(bb);
    assert(status1 == NodeStatus::Success);
    assert(bb.get_string("current_action") == "patrol");

    // Tick 2: Target distance is now 3.0f -> Condition succeeds, Combat sequence executes
    bb.set_float("target_distance", 3.0f);
    NodeStatus status2 = root.tick(bb);
    assert(status2 == NodeStatus::Success);
    assert(bb.get_string("current_action") == "melee_attack");

    // Test Inverter decorator
    auto cond = std::make_shared<ConditionNode>([](Blackboard& b) {
        return b.get_float("target_distance") < 1.0f;
    });
    InverterNode inv(cond);
    assert(inv.tick(bb) == NodeStatus::Success); // Condition was false, inverter turns it true

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
    test_state_machine_stack_push_pop();
    test_behavior_tree_execution();

    std::cout << "=== All 9 game-loop-fsm tests passed successfully! ===\n";
    return 0;
}

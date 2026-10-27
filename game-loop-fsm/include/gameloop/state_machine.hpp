#pragma once

#include "event_bus.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace gamedev::gameloop {

class StateMachine;

/**
 * Base abstract State for Hierarchical Finite State Machines.
 */
class State {
public:
    virtual ~State() = default;

    virtual void on_enter(StateMachine& /*fsm*/) {}
    virtual void on_update(StateMachine& /*fsm*/, float /*dt*/) {}
    virtual void on_exit(StateMachine& /*fsm*/) {}
    virtual void handle_input(StateMachine& /*fsm*/, const std::string& /*input*/) {}

    virtual std::string name() const = 0;
};

/**
 * Finite State Machine orchestrator managing active state and transitions.
 */
class StateMachine {
public:
    explicit StateMachine(EventBus* event_bus = nullptr) : event_bus_(event_bus) {}

    void change_state(std::unique_ptr<State> new_state) {
        if (!new_state) return;

        std::string prev_name = current_state_ ? current_state_->name() : "None";
        std::string new_name = new_state->name();

        if (current_state_) {
            current_state_->on_exit(*this);
        }

        current_state_ = std::move(new_state);
        history_.push_back(new_name);

        if (event_bus_) {
            event_bus_->publish(StateChangedEvent{prev_name, new_name});
        }

        current_state_->on_enter(*this);
    }

    void update(float dt) {
        if (current_state_) {
            current_state_->on_update(*this, dt);
        }
    }

    void handle_input(const std::string& input) {
        if (current_state_) {
            current_state_->handle_input(*this, input);
        }
    }

    std::string current_state_name() const {
        return current_state_ ? current_state_->name() : "None";
    }

    const std::vector<std::string>& history() const noexcept {
        return history_;
    }

    EventBus* event_bus() const noexcept {
        return event_bus_;
    }

    // Shared character context data across states
    struct CharacterContext {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        bool is_grounded = true;
    } context;

private:
    std::unique_ptr<State> current_state_;
    EventBus* event_bus_ = nullptr;
    std::vector<std::string> history_;
};

// Forward declaration of states
class IdleState;
class WalkState;
class JumpState;
class FallState;
class AttackState;

// ============================================================================
// Concrete Character States
// ============================================================================

class IdleState : public State {
public:
    std::string name() const override { return "Idle"; }

    void on_enter(StateMachine& fsm) override {
        fsm.context.vx = 0.0f;
    }

    void handle_input(StateMachine& fsm, const std::string& input) override;
};

class WalkState : public State {
public:
    std::string name() const override { return "Walk"; }

    void on_enter(StateMachine& fsm) override {
        fsm.context.vx = 4.0f; // Walk speed
    }

    void on_update(StateMachine& fsm, float dt) override {
        fsm.context.x += fsm.context.vx * dt;
    }

    void handle_input(StateMachine& fsm, const std::string& input) override;
};

class JumpState : public State {
public:
    std::string name() const override { return "Jump"; }

    void on_enter(StateMachine& fsm) override {
        fsm.context.is_grounded = false;
        fsm.context.vy = 8.0f; // Upward jump impulse
        if (fsm.event_bus()) {
            fsm.event_bus()->publish(PlayerJumpedEvent{1, 8.0f});
        }
    }

    void on_update(StateMachine& fsm, float dt) override;
    void handle_input(StateMachine& fsm, const std::string& input) override;
};

class FallState : public State {
public:
    std::string name() const override { return "Fall"; }

    void on_enter(StateMachine& fsm) override {
        fsm.context.is_grounded = false;
    }

    void on_update(StateMachine& fsm, float dt) override;
};

class AttackState : public State {
public:
    std::string name() const override { return "Attack"; }

    void on_enter(StateMachine& fsm) override {
        duration_remaining_ = 0.25f; // 250ms attack animation lock
        if (fsm.event_bus()) {
            fsm.event_bus()->publish(AttackExecutedEvent{1, "Slash", 35});
        }
    }

    void on_update(StateMachine& fsm, float dt) override;

private:
    float duration_remaining_ = 0.25f;
};

// Implementations of state transitions

inline void IdleState::handle_input(StateMachine& fsm, const std::string& input) {
    if (input == "move_right" || input == "move_left") {
        fsm.change_state(std::make_unique<WalkState>());
    } else if (input == "jump" && fsm.context.is_grounded) {
        fsm.change_state(std::make_unique<JumpState>());
    } else if (input == "attack") {
        fsm.change_state(std::make_unique<AttackState>());
    }
}

inline void WalkState::handle_input(StateMachine& fsm, const std::string& input) {
    if (input == "stop") {
        fsm.change_state(std::make_unique<IdleState>());
    } else if (input == "jump" && fsm.context.is_grounded) {
        fsm.change_state(std::make_unique<JumpState>());
    } else if (input == "attack") {
        fsm.change_state(std::make_unique<AttackState>());
    }
}

inline void JumpState::on_update(StateMachine& fsm, float dt) {
    fsm.context.vy -= 19.62f * dt; // Gravity
    fsm.context.y += fsm.context.vy * dt;
    fsm.context.x += fsm.context.vx * dt;

    if (fsm.context.vy <= 0.0f) {
        fsm.change_state(std::make_unique<FallState>());
    }
}

inline void JumpState::handle_input(StateMachine& fsm, const std::string& input) {
    if (input == "attack") {
        fsm.change_state(std::make_unique<AttackState>());
    }
}

inline void FallState::on_update(StateMachine& fsm, float dt) {
    fsm.context.vy -= 19.62f * dt; // Gravity
    fsm.context.y += fsm.context.vy * dt;
    fsm.context.x += fsm.context.vx * dt;

    // Check ground collision
    if (fsm.context.y <= 0.0f) {
        fsm.context.y = 0.0f;
        fsm.context.vy = 0.0f;
        fsm.context.is_grounded = true;

        if (fsm.event_bus()) {
            fsm.event_bus()->publish(PlayerLandedEvent{1, std::abs(fsm.context.vy)});
        }

        if (std::abs(fsm.context.vx) > 0.1f) {
            fsm.change_state(std::make_unique<WalkState>());
        } else {
            fsm.change_state(std::make_unique<IdleState>());
        }
    }
}

inline void AttackState::on_update(StateMachine& fsm, float dt) {
    duration_remaining_ -= dt;
    if (duration_remaining_ <= 0.0f) {
        if (fsm.context.is_grounded) {
            fsm.change_state(std::make_unique<IdleState>());
        } else {
            fsm.change_state(std::make_unique<FallState>());
        }
    }
}

} // namespace gamedev::gameloop

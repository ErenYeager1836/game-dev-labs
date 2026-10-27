#pragma once

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace gamedev::gameloop {

class EventBus {
public:
    template <typename EventT>
    using Handler = std::function<void(const EventT&)>;

    template <typename EventT>
    void subscribe(Handler<EventT> handler) {
        auto type = std::type_index(typeid(EventT));
        auto wrapper = [handler](const void* event_ptr) {
            handler(*static_cast<const EventT*>(event_ptr));
        };
        handlers_[type].push_back(wrapper);
    }

    template <typename EventT>
    void publish(const EventT& event) {
        auto type = std::type_index(typeid(EventT));
        auto it = handlers_.find(type);
        if (it != handlers_.end()) {
            for (const auto& handler : it->second) {
                handler(&event);
            }
        }
    }

    void clear() {
        handlers_.clear();
    }

private:
    using GenericHandler = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<GenericHandler>> handlers_;
};

// Common gameplay events
struct PlayerJumpedEvent {
    int player_id;
    float vertical_velocity;
};

struct PlayerLandedEvent {
    int player_id;
    float impact_speed;
};

struct AttackExecutedEvent {
    int player_id;
    std::string attack_type;
    int base_damage;
};

struct StateChangedEvent {
    std::string from_state;
    std::string to_state;
};

} // namespace gamedev::gameloop

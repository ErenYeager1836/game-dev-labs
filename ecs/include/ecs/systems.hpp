#pragma once

#include "components.hpp"
#include "registry.hpp"

#include <iostream>
#include <vector>

namespace gamedev::ecs {

class MovementSystem {
public:
    static size_t update(Registry& registry, float dt) {
        size_t updated = 0;
        registry.view<Position, Velocity>([&](EntityID, Position& pos, const Velocity& vel) {
            pos.x += vel.vx * dt;
            pos.y += vel.vy * dt;
            ++updated;
        });
        return updated;
    }
};

class CombatSystem {
public:
    static void apply_damage(Registry& registry, EntityID target, int damage) {
        if (registry.has<Health>(target)) {
            auto& hp = registry.get<Health>(target);
            hp.current = std::max(0, hp.current - damage);
        }
    }

    static std::vector<EntityID> cleanup_dead(Registry& registry) {
        std::vector<EntityID> dead_entities;
        registry.view<Health>([&](EntityID entity, const Health& hp) {
            if (hp.is_dead()) {
                dead_entities.push_back(entity);
            }
        });

        for (EntityID entity : dead_entities) {
            registry.destroy(entity);
        }
        return dead_entities;
    }
};

} // namespace gamedev::ecs

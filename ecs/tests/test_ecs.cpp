#include "ecs/components.hpp"
#include "ecs/registry.hpp"
#include "ecs/systems.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace gamedev::ecs;

void test_entity_creation_destruction_and_recycling() {
    std::cout << "[+] Running test_entity_creation_destruction_and_recycling...\n";
    Registry registry;

    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    EntityID e3 = registry.create();

    assert(e1 != e2 && e2 != e3);
    assert(registry.is_alive(e1));
    assert(registry.is_alive(e2));
    assert(registry.is_alive(e3));
    assert(registry.alive_count() == 3);

    // Destroy e2
    registry.destroy(e2);
    assert(!registry.is_alive(e2));
    assert(registry.alive_count() == 2);

    // New entity should recycle e2
    EntityID e4 = registry.create();
    assert(e4 == e2);
    assert(registry.is_alive(e4));
    assert(registry.alive_count() == 3);

    std::cout << "    PASSED\n";
}

void test_component_emplace_get_and_has() {
    std::cout << "[+] Running test_component_emplace_get_and_has...\n";
    Registry registry;

    EntityID e = registry.create();
    assert(!registry.has<Position>(e));
    assert(!registry.has<Velocity>(e));

    registry.emplace<Position>(e, 10.0f, 20.0f);
    assert(registry.has<Position>(e));
    assert(!registry.has<Velocity>(e));

    Position& pos = registry.get<Position>(e);
    assert(pos.x == 10.0f && pos.y == 20.0f);

    pos.x = 42.0f;
    assert(registry.get<Position>(e).x == 42.0f);

    registry.emplace<Velocity>(e, -5.0f, 2.5f);
    assert((registry.has_all<Position, Velocity>(e)));
    assert(registry.get<Velocity>(e).vx == -5.0f);

    std::cout << "    PASSED\n";
}

void test_sparse_set_swap_and_pop_integrity() {
    std::cout << "[+] Running test_sparse_set_swap_and_pop_integrity...\n";
    Registry registry;

    std::vector<EntityID> entities;
    for (int i = 0; i < 5; ++i) {
        EntityID e = registry.create();
        registry.emplace<Position>(e, static_cast<float>(i * 10), static_cast<float>(i * 20));
        entities.push_back(e);
    }

    assert(registry.pool<Position>().size() == 5);

    // Remove middle entity (index 2)
    EntityID removed_entity = entities[2];
    assert(registry.remove<Position>(removed_entity));
    assert(!registry.has<Position>(removed_entity));
    assert(registry.pool<Position>().size() == 4);

    // Remaining entities must remain valid and point to correct data
    assert(registry.has<Position>(entities[0]));
    assert(registry.has<Position>(entities[1]));
    assert(registry.has<Position>(entities[3]));
    assert(registry.has<Position>(entities[4]));

    assert(registry.get<Position>(entities[0]).x == 0.0f);
    assert(registry.get<Position>(entities[1]).x == 10.0f);
    assert(registry.get<Position>(entities[3]).x == 30.0f);
    assert(registry.get<Position>(entities[4]).x == 40.0f);

    std::cout << "    PASSED\n";
}

void test_multi_component_view_filtering() {
    std::cout << "[+] Running test_multi_component_view_filtering...\n";
    Registry registry;

    EntityID e1 = registry.create(); // Has Position & Velocity
    registry.emplace<Position>(e1, 0.0f, 0.0f);
    registry.emplace<Velocity>(e1, 1.0f, 1.0f);

    EntityID e2 = registry.create(); // Has Position only
    registry.emplace<Position>(e2, 5.0f, 5.0f);

    EntityID e3 = registry.create(); // Has Position & Velocity
    registry.emplace<Position>(e3, 10.0f, 10.0f);
    registry.emplace<Velocity>(e3, 2.0f, 2.0f);

    EntityID e4 = registry.create(); // Has Velocity only
    registry.emplace<Velocity>(e4, 3.0f, 3.0f);

    size_t count = 0;
    registry.view<Position, Velocity>([&](EntityID e, Position& p, const Velocity& v) {
        assert(e == e1 || e == e3);
        p.x += v.vx;
        p.y += v.vy;
        ++count;
    });

    assert(count == 2);
    assert(registry.get<Position>(e1).x == 1.0f);
    assert(registry.get<Position>(e2).x == 5.0f); // Unmodified
    assert(registry.get<Position>(e3).x == 12.0f);

    std::cout << "    PASSED\n";
}

void test_system_movement_and_combat() {
    std::cout << "[+] Running test_system_movement_and_combat...\n";
    Registry registry;

    EntityID hero = registry.create();
    registry.emplace<Position>(hero, 100.0f, 200.0f);
    registry.emplace<Velocity>(hero, 10.0f, -20.0f);
    registry.emplace<Health>(hero, 100, 100);

    EntityID enemy = registry.create();
    registry.emplace<Position>(enemy, 50.0f, 50.0f);
    registry.emplace<Health>(enemy, 30, 30);

    // Run Movement
    size_t moved = MovementSystem::update(registry, 0.5f);
    assert(moved == 1);
    assert(registry.get<Position>(hero).x == 105.0f);
    assert(registry.get<Position>(hero).y == 190.0f);

    // Run Combat: non-lethal damage
    CombatSystem::apply_damage(registry, enemy, 10);
    assert(registry.get<Health>(enemy).current == 20);
    auto dead = CombatSystem::cleanup_dead(registry);
    assert(dead.empty());
    assert(registry.is_alive(enemy));

    // Run Combat: lethal damage
    CombatSystem::apply_damage(registry, enemy, 50);
    assert(registry.get<Health>(enemy).current == 0);
    dead = CombatSystem::cleanup_dead(registry);
    assert(dead.size() == 1 && dead[0] == enemy);
    assert(!registry.is_alive(enemy));

    std::cout << "    PASSED\n";
}

void test_high_volume_stress() {
    std::cout << "[+] Running test_high_volume_stress...\n";
    Registry registry;
    constexpr size_t TOTAL = 10000;

    for (size_t i = 0; i < TOTAL; ++i) {
        EntityID e = registry.create();
        registry.emplace<Position>(e, static_cast<float>(i), static_cast<float>(i));
        registry.emplace<Velocity>(e, 1.0f, 1.0f);
    }

    assert(registry.alive_count() == TOTAL);
    size_t updated = MovementSystem::update(registry, 1.0f);
    assert(updated == TOTAL);

    // Verify first and last
    assert(registry.get<Position>(0).x == 1.0f);
    assert(registry.get<Position>(TOTAL - 1).x == static_cast<float>(TOTAL));

    // Destroy half of the entities
    for (size_t i = 0; i < TOTAL; i += 2) {
        registry.destroy(static_cast<EntityID>(i));
    }

    assert(registry.alive_count() == TOTAL / 2);
    size_t updated_half = MovementSystem::update(registry, 1.0f);
    assert(updated_half == TOTAL / 2);

    std::cout << "    PASSED\n";
}

void test_component_lifecycle_signals() {
    std::cout << "[+] Running test_component_lifecycle_signals...\n";
    Registry registry;

    std::vector<std::pair<EntityID, int>> constructs;
    std::vector<std::pair<EntityID, int>> destructions;

    registry.on_construct<Health>([&](EntityID entity, Health& h) {
        constructs.emplace_back(entity, h.current);
    });

    registry.on_destroy<Health>([&](EntityID entity, Health& h) {
        destructions.emplace_back(entity, h.current);
    });

    EntityID e1 = registry.create();
    EntityID e2 = registry.create();

    registry.emplace<Health>(e1, 100, 100);
    registry.emplace<Health>(e2, 50, 50);

    assert(constructs.size() == 2);
    assert(constructs[0].first == e1 && constructs[0].second == 100);
    assert(constructs[1].first == e2 && constructs[1].second == 50);

    // Remove component explicitly from e1
    bool removed = registry.remove<Health>(e1);
    assert(removed);
    assert(destructions.size() == 1);
    assert(destructions[0].first == e1 && destructions[0].second == 100);

    // Destroy e2 entity, which purges all its components
    registry.destroy(e2);
    assert(destructions.size() == 2);
    assert(destructions[1].first == e2 && destructions[1].second == 50);

    std::cout << "    PASSED\n";
}

int main() {
    std::cout << "=== Running ecs test suite ===\n";

    test_entity_creation_destruction_and_recycling();
    test_component_emplace_get_and_has();
    test_sparse_set_swap_and_pop_integrity();
    test_multi_component_view_filtering();
    test_system_movement_and_combat();
    test_high_volume_stress();
    test_component_lifecycle_signals();

    std::cout << "=== All 7 ecs tests passed successfully! ===\n";
    return 0;
}

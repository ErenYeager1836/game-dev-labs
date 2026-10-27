#pragma once

#include "component_pool.hpp"
#include "entity.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace gamedev::ecs {

/**
 * ECS Registry coordinating entities, component storage pools, and multi-component view queries.
 */
class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;

    /**
     * Spawns a new active entity, reusing recycled IDs if available.
     */
    EntityID create() {
        EntityID id;
        if (!free_entities_.empty()) {
            id = free_entities_.back();
            free_entities_.pop_back();
        } else {
            id = next_entity_id_++;
        }

        if (id >= entity_alive_.size()) {
            entity_alive_.resize(id + 1, false);
        }
        entity_alive_[id] = true;
        ++alive_count_;
        return id;
    }

    /**
     * Destroys an entity and purges all of its associated components across all pools.
     */
    void destroy(EntityID entity) {
        if (!is_alive(entity)) {
            return;
        }

        for (auto& [type, pool] : pools_) {
            pool->remove(entity);
        }

        entity_alive_[entity] = false;
        free_entities_.push_back(entity);
        --alive_count_;
    }

    /**
     * Checks if an entity is currently alive.
     */
    bool is_alive(EntityID entity) const noexcept {
        return entity < entity_alive_.size() && entity_alive_[entity];
    }

    size_t alive_count() const noexcept {
        return alive_count_;
    }

    /**
     * Assigns or constructs a component in-place for the specified entity.
     */
    template <typename T, typename... Args>
    T& emplace(EntityID entity, Args&&... args) {
        assert(is_alive(entity) && "Cannot add component to a non-alive entity");
        return get_or_create_pool<T>().emplace(entity, std::forward<Args>(args)...);
    }

    /**
     * Returns a mutable reference to the entity's component.
     */
    template <typename T>
    T& get(EntityID entity) {
        assert(is_alive(entity) && "Cannot get component from a non-alive entity");
        return get_or_create_pool<T>().get(entity);
    }

    /**
     * Returns a const reference to the entity's component.
     */
    template <typename T>
    const T& get(EntityID entity) const {
        assert(is_alive(entity) && "Cannot get component from a non-alive entity");
        auto it = pools_.find(std::type_index(typeid(T)));
        if (it == pools_.end()) {
            throw std::runtime_error("Component pool not registered");
        }
        return static_cast<const ComponentPool<T>*>(it->second.get())->get(entity);
    }

    /**
     * Checks if an entity owns component T.
     */
    template <typename T>
    bool has(EntityID entity) const noexcept {
        if (!is_alive(entity)) {
            return false;
        }
        auto it = pools_.find(std::type_index(typeid(T)));
        if (it == pools_.end()) {
            return false;
        }
        return it->second->has(entity);
    }

    /**
     * Checks if an entity owns all requested component types simultaneously.
     */
    template <typename... Components>
    bool has_all(EntityID entity) const noexcept {
        return (has<Components>(entity) && ...);
    }

    /**
     * Removes component T from entity.
     */
    template <typename T>
    bool remove(EntityID entity) {
        if (!is_alive(entity)) {
            return false;
        }
        auto it = pools_.find(std::type_index(typeid(T)));
        if (it == pools_.end()) {
            return false;
        }
        return it->second->remove(entity);
    }

    /**
     * Retrieves the component pool for a given component type.
     */
    template <typename T>
    ComponentPool<T>& pool() {
        return get_or_create_pool<T>();
    }

    /**
     * Multi-component view query executing a function over every entity that has all specified components.
     * Iterates through the first/primary component pool and checks presence in other pools.
     *
     * Example:
     *   registry.view<Position, Velocity>([](EntityID e, Position& pos, Velocity& vel) {
     *       pos.x += vel.vx * dt;
     *   });
     */
    template <typename First, typename... Rest, typename Func>
    void view(Func&& func) {
        auto& primary_pool = get_or_create_pool<First>();
        const auto& entities = primary_pool.entities();

        for (EntityID entity : entities) {
            if ((has<Rest>(entity) && ...)) {
                func(entity, primary_pool.get(entity), get<Rest>(entity)...);
            }
        }
    }

    /**
     * Const view query over immutable components.
     */
    template <typename First, typename... Rest, typename Func>
    void view(Func&& func) const {
        auto it = pools_.find(std::type_index(typeid(First)));
        if (it == pools_.end()) {
            return;
        }
        auto* primary_pool = static_cast<const ComponentPool<First>*>(it->second.get());
        const auto& entities = primary_pool->entities();

        for (EntityID entity : entities) {
            if ((has<Rest>(entity) && ...)) {
                func(entity, primary_pool->get(entity), get<Rest>(entity)...);
            }
        }
    }

    /**
     * Clears all entities and component pools.
     */
    void clear() {
        for (auto& [type, pool] : pools_) {
            pool->clear();
        }
        entity_alive_.clear();
        free_entities_.clear();
        next_entity_id_ = 0;
        alive_count_ = 0;
    }

private:
    template <typename T>
    ComponentPool<T>& get_or_create_pool() {
        auto type = std::type_index(typeid(T));
        auto it = pools_.find(type);
        if (it == pools_.end()) {
            auto new_pool = std::make_unique<ComponentPool<T>>();
            auto* raw = new_pool.get();
            pools_[type] = std::move(new_pool);
            return *raw;
        }
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }

    EntityID next_entity_id_ = 0;
    size_t alive_count_ = 0;
    std::vector<bool> entity_alive_;
    std::vector<EntityID> free_entities_;
    std::unordered_map<std::type_index, std::unique_ptr<IPool>> pools_;
};

} // namespace gamedev::ecs

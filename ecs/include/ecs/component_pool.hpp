#pragma once

#include "entity.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <utility>
#include <vector>

namespace gamedev::ecs {

/**
 * Type-erased base interface for component storage pools.
 * Allows Registry to remove an entity from all registered pools without knowing component types.
 */
class IPool {
public:
    virtual ~IPool() = default;
    virtual bool has(EntityID entity) const = 0;
    virtual bool remove(EntityID entity) = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
};

/**
 * Cache-friendly Sparse Set component pool.
 *
 * Data layout:
 * - dense_: Contiguous array of component data (T). Highly cache-efficient for system iteration.
 * - dense_to_entity_: Parallel array mapping each dense index back to its owner EntityID.
 * - sparse_: Direct-mapped array indexed by EntityID pointing to the corresponding index in dense_.
 *
 * Performance guarantees:
 * - Insertion: O(1) amortized
 * - Lookup: O(1) direct indexing
 * - Removal: O(1) via swap-and-pop (replaces deleted element with the last element)
 * - Iteration: O(N) packed linear cache traversal with zero holes
 */
template <typename T>
class ComponentPool : public IPool {
public:
    ComponentPool() = default;

    bool has(EntityID entity) const override {
        if (entity >= sparse_.size()) {
            return false;
        }
        size_t dense_idx = sparse_[entity];
        return dense_idx < dense_.size() && dense_to_entity_[dense_idx] == entity;
    }

    using Callback = std::function<void(EntityID, T&)>;

    void on_construct(Callback cb) {
        on_construct_callbacks_.push_back(std::move(cb));
    }

    void on_destroy(Callback cb) {
        on_destroy_callbacks_.push_back(std::move(cb));
    }

    template <typename... Args>
    T& emplace(EntityID entity, Args&&... args) {
        if (has(entity)) {
            // Overwrite existing component
            size_t idx = sparse_[entity];
            dense_[idx] = T(std::forward<Args>(args)...);
            for (auto& cb : on_construct_callbacks_) {
                cb(entity, dense_[idx]);
            }
            return dense_[idx];
        }

        // Expand sparse array if entity ID exceeds current capacity
        if (entity >= sparse_.size()) {
            sparse_.resize(entity + 1, INVALID_DENSE_INDEX);
        }

        size_t new_dense_idx = dense_.size();
        sparse_[entity] = new_dense_idx;
        dense_.emplace_back(std::forward<Args>(args)...);
        dense_to_entity_.push_back(entity);

        for (auto& cb : on_construct_callbacks_) {
            cb(entity, dense_.back());
        }

        return dense_.back();
    }

    T& get(EntityID entity) {
        assert(has(entity) && "Attempted to access non-existent component on Entity");
        return dense_[sparse_[entity]];
    }

    const T& get(EntityID entity) const {
        assert(has(entity) && "Attempted to access non-existent component on Entity");
        return dense_[sparse_[entity]];
    }

    bool remove(EntityID entity) override {
        if (!has(entity)) {
            return false;
        }

        size_t index_to_remove = sparse_[entity];

        // Notify destruction listeners before removing component
        for (auto& cb : on_destroy_callbacks_) {
            cb(entity, dense_[index_to_remove]);
        }

        size_t last_index = dense_.size() - 1;

        // Swap with the last element to keep dense_ tightly packed with no fragmentation
        if (index_to_remove != last_index) {
            dense_[index_to_remove] = std::move(dense_[last_index]);
            EntityID last_entity = dense_to_entity_[last_index];
            dense_to_entity_[index_to_remove] = last_entity;
            sparse_[last_entity] = index_to_remove;
        }

        dense_.pop_back();
        dense_to_entity_.pop_back();
        sparse_[entity] = INVALID_DENSE_INDEX;

        return true;
    }

    void clear() override {
        dense_.clear();
        dense_to_entity_.clear();
        sparse_.clear();
    }

    size_t size() const override {
        return dense_.size();
    }

    // Direct access to contiguous storage for high-performance vectorized iteration
    std::vector<T>& data() noexcept { return dense_; }
    const std::vector<T>& data() const noexcept { return dense_; }
    const std::vector<EntityID>& entities() const noexcept { return dense_to_entity_; }

private:
    static constexpr size_t INVALID_DENSE_INDEX = std::numeric_limits<size_t>::max();

    std::vector<T> dense_;
    std::vector<EntityID> dense_to_entity_;
    std::vector<size_t> sparse_;
    std::vector<Callback> on_construct_callbacks_;
    std::vector<Callback> on_destroy_callbacks_;
};

} // namespace gamedev::ecs

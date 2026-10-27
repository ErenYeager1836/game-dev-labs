#pragma once

#include <cstdint>
#include <limits>

namespace gamedev::ecs {

using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();

/**
 * Entity identifier structure.
 * Can be represented as a clean 32-bit integer or unpacked into index and generation
 * for safe entity identifier recycling.
 */
struct Entity {
    EntityID id = INVALID_ENTITY;

    constexpr Entity() = default;
    constexpr explicit Entity(EntityID id_) : id(id_) {}

    constexpr bool is_valid() const noexcept { return id != INVALID_ENTITY; }
    constexpr bool operator==(const Entity& other) const noexcept { return id == other.id; }
    constexpr bool operator!=(const Entity& other) const noexcept { return id != other.id; }
    constexpr bool operator<(const Entity& other) const noexcept { return id < other.id; }
    constexpr explicit operator EntityID() const noexcept { return id; }
};

} // namespace gamedev::ecs

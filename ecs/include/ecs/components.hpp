#pragma once

#include <string>

namespace gamedev::ecs {

struct Position {
    float x = 0.0f;
    float y = 0.0f;

    Position() = default;
    Position(float x_, float y_) : x(x_), y(y_) {}
};

struct Velocity {
    float vx = 0.0f;
    float vy = 0.0f;

    Velocity() = default;
    Velocity(float vx_, float vy_) : vx(vx_), vy(vy_) {}
};

struct Health {
    int current = 100;
    int max = 100;

    Health() = default;
    Health(int current_, int max_) : current(current_), max(max_) {}

    bool is_dead() const noexcept { return current <= 0; }
};

struct Renderable {
    char glyph = '@';
    uint32_t color_code = 0xFFFFFF;
    int z_order = 0;

    Renderable() = default;
    Renderable(char glyph_, uint32_t color_, int z_order_ = 0)
        : glyph(glyph_), color_code(color_), z_order(z_order_) {}
};

struct NameTag {
    std::string name;

    NameTag() = default;
    explicit NameTag(std::string name_) : name(std::move(name_)) {}
};

} // namespace gamedev::ecs

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>

namespace gamedev::gameloop {

/**
 * Fixed-timestep accumulator implementation ("Fix Your Timestep!").
 *
 * Guarantees completely deterministic physics updates at a fixed frequency (e.g. 60Hz),
 * decoupled from variable graphics render frame rates.
 */
class TimestepManager {
public:
    explicit TimestepManager(double fixed_dt = 1.0 / 60.0, double max_frame_time = 0.25)
        : fixed_dt_(fixed_dt), max_frame_time_(max_frame_time), accumulator_(0.0) {}

    double fixed_dt() const noexcept { return fixed_dt_; }
    double accumulator() const noexcept { return accumulator_; }
    uint64_t total_ticks() const noexcept { return total_ticks_; }

    /**
     * Consumes frame delta time, executing fixed update callbacks as many times
     * as required, and computes the fractional alpha interpolation factor.
     */
    template <typename UpdateFunc>
    void process_frame(double frame_delta_time, UpdateFunc&& fixed_update_fn) {
        // Clamp frame time to prevent the "spiral of death" if a frame takes too long
        double clamped_delta = std::min(frame_delta_time, max_frame_time_);
        accumulator_ += clamped_delta;

        while (accumulator_ >= fixed_dt_) {
            fixed_update_fn(static_cast<float>(fixed_dt_));
            accumulator_ -= fixed_dt_;
            total_ticks_++;
        }
    }

    /**
     * Returns the render interpolation blending factor alpha in [0.0, 1.0).
     * Used to interpolate between previous and current physics states for jitter-free rendering:
     *   render_state = previous_state * (1 - alpha) + current_state * alpha
     */
    float interpolation_alpha() const noexcept {
        return static_cast<float>(accumulator_ / fixed_dt_);
    }

    void reset() noexcept {
        accumulator_ = 0.0;
        total_ticks_ = 0;
    }

private:
    double fixed_dt_;        // Fixed simulation step (e.g. 0.016667s for 60 Hz)
    double max_frame_time_;   // Maximum clamped delta time preventing infinite update loops
    double accumulator_;     // Stored fractional delta time
    uint64_t total_ticks_ = 0;
};

/**
 * Linear interpolation helper.
 */
template <typename T>
inline constexpr T lerp(const T& a, const T& b, float alpha) noexcept {
    return a + (b - a) * alpha;
}

} // namespace gamedev::gameloop

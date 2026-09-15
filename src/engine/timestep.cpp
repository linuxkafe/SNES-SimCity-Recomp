#include "engine/timestep.h"

#include <cmath>

namespace engine {

static constexpr double kNano = 1e9;

FixedTimestep::FixedTimestep(double step_fps, double max_burst_seconds)
    : interval_s_(1.0 / step_fps),
      interval_ns_(static_cast<uint64_t>(std::llround(interval_s_ * kNano))),
      max_accum_ns_(static_cast<uint64_t>(max_burst_seconds * kNano)) {}

int FixedTimestep::accumulate(double elapsed_seconds) {
    uint64_t dt = static_cast<uint64_t>(elapsed_seconds * kNano);
    accumulator_ns_ += dt;
    if (accumulator_ns_ > max_accum_ns_) accumulator_ns_ = max_accum_ns_;
    int ticks = 0;
    while (accumulator_ns_ >= interval_ns_) {
        accumulator_ns_ -= interval_ns_;
        ++ticks;
    }
    return ticks;
}

} // namespace engine
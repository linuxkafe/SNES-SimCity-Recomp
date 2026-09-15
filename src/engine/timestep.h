#pragma once

#include <cstdint>

namespace engine {

// Deterministic fixed-timestep accumulator, integer-nanosecond based to avoid
// floating-point drift. Feed it wall-clock deltas; it returns how many fixed
// ticks are due. Use with a dt of 1/step_fps.
class FixedTimestep {
public:
    explicit FixedTimestep(double step_fps, double max_burst_seconds = 0.25);

    // Returns number of fixed ticks due for the given elapsed seconds.
    // Clamps to avoid spiral-of-death bursts.
    int accumulate(double elapsed_seconds);

    double interval_seconds() const { return interval_s_; }

private:
    double   interval_s_;
    uint64_t interval_ns_;
    uint64_t accumulator_ns_ = 0;
    uint64_t max_accum_ns_;
};

} // namespace engine
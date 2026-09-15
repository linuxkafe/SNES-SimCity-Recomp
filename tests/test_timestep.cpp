#include <cmath>
#include <cstdio>

#include "engine/timestep.h"

static int failures = 0;
#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)
#define CHECK_NEAR(a, b)                                                    \
    do {                                                                    \
        double _a = (a), _b = (b);                                          \
        if (std::fabs(_a - _b) > 1e-9) {                                    \
            printf("FAIL: |%f - %f| too large (line %d)\n", _a, _b, __LINE__);\
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static void test_50hz_steps() {
    printf("-- test_50hz_steps\n");
    engine::FixedTimestep ts(50.0);
    // one tick's worth of time (0.02s) → 1 tick
    int ticks = ts.accumulate(0.02);
    CHECK(ticks == 1);
    // advance one simulated second in 50 separate frames → 50 ticks total
    engine::FixedTimestep ts2(50.0);
    int total = 0;
    for (int i = 0; i < 50; ++i) total += ts2.accumulate(0.02);
    CHECK(total == 50);
}

static void test_burst_clamp() {
    printf("-- test_burst_clamp\n");
    // huge frame → at most max_burst_seconds worth of ticks
    engine::FixedTimestep ts(50.0, 0.25);
    int ticks = ts.accumulate(10.0);
    CHECK(ticks == 12);  // 0.25 / 0.02 = 12.5 → floors to 12... but loop logic: accumulator clamped to 0.25, then 0.25/0.02 = 12 ticks with 0.01 remainder
    // Verify interval
    CHECK_NEAR(ts.interval_seconds(), 0.02);
}

static void test_partial_accum() {
    printf("-- test_partial_accum\n");
    engine::FixedTimestep ts(50.0);
    int ticks = ts.accumulate(0.01);  // less than one tick
    CHECK(ticks == 0);
    ticks = ts.accumulate(0.01);      // accumulates to one tick
    CHECK(ticks == 1);
}

int main() {
    printf("Timestep test suite\n");
    test_50hz_steps();
    test_burst_clamp();
    test_partial_accum();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}
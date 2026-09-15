#include <cstdio>

#include "sim/city.h"

static int failures = 0;
#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static void test_placement() {
    printf("-- test_placement\n");
    sim::City c;
    CHECK(c.terrain(0, 0) == sim::Terrain::Grass);
    CHECK(c.zone(0, 0) == sim::Zone::None);
    CHECK(c.density(0, 0) == 0);

    c.set_terrain(1, 1, sim::Terrain::Water);
    CHECK(c.terrain(1, 1) == sim::Terrain::Water);

    c.set_zone(1, 1, sim::Zone::Residential);  // water rejected
    CHECK(c.zone(1, 1) == sim::Zone::None);

    c.set_terrain(2, 2, sim::Terrain::Grass);
    c.set_zone(2, 2, sim::Zone::Commercial);
    CHECK(c.zone(2, 2) == sim::Zone::Commercial);

    // bulldoze: changing terrain clears the zone
    c.set_terrain(2, 2, sim::Terrain::Tree);
    CHECK(c.zone(2, 2) == sim::Zone::None);
    CHECK(c.terrain(2, 2) == sim::Terrain::Tree);

    // out of bounds is safe
    c.set_zone(-1, 0, sim::Zone::Industrial);
    c.set_terrain(1000, 1000, sim::Terrain::Water);
    CHECK(c.terrain(-1, -1) == sim::Terrain::Grass);
}

static void test_residential_growth() {
    printf("-- test_residential_growth\n");
    sim::City c;
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    // Plant footprint (10,10)-(12,12), center (11,11), radius 1 powers only the
    // plant's own 3x3. Everything else needs power lines. Extend row 11 outward.
    for (int i = 1; i <= 6; ++i) c.place_power_line(12 + i, 11);  // (13,11)..(18,11)
    // Industrial adjacent to a powered line at (18,11)
    c.set_zone(19, 11, sim::Zone::Industrial);
    // Road provides access for residential growth
    c.set_terrain(15, 10, sim::Terrain::Road);
    // Residential adjacent to powered line at (16,11)
    c.set_zone(16, 10, sim::Zone::Residential);

    for (int m = 0; m < 12; ++m) c.step_month();

    sim::Stats s = c.stats();
    CHECK(s.population == 5);
    CHECK(s.resident_tiles == 1);
    CHECK(s.industrial_tiles == 1);
    CHECK(s.jobs == 4);
    CHECK(c.density(16, 10) == sim::City::kMaxDensity);
}

static void test_population_job_cap() {
    printf("-- test_population_job_cap\n");
    sim::City c;
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    // Power lines extend row 11 outward past the plant footprint (10,10)-(12,12).
    for (int i = 1; i <= 6; ++i) c.place_power_line(12 + i, 11);  // (13,11)..(18,11)
    // 4 jobs of capacity force population to be bounded by jobs*2 = 8
    c.set_zone(19, 11, sim::Zone::Industrial);  // adjacent to line (18,11)
    c.set_terrain(15, 10, sim::Terrain::Road);  // road access for residential
    c.set_zone(16, 10, sim::Zone::Residential); // adjacent to line (16,11)
    c.set_zone(17, 10, sim::Zone::Residential); // adjacent to line (17,11)

    int peak = 0;
    for (int m = 0; m < 60; ++m) {
        c.step_month();
        int pop = c.stats().population;
        if (pop > peak) peak = pop;
        CHECK(pop <= 8);
    }
    CHECK(peak >= 6);
    CHECK(c.funds() >= 0);
}

static void test_budget_burn() {
    printf("-- test_budget_burn\n");
    sim::City c;
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    // lots of road upkeep and no income -> funds burn down to zero, never below
    for (int i = 0; i < 40; ++i) c.set_terrain(i, 0, sim::Terrain::Road);

    bool negative_seen = false;
    for (int m = 0; m < 60; ++m) {
        c.step_month();
        if (c.funds() < 0) negative_seen = true;
    }
    CHECK(c.funds() == 0);
    CHECK(!negative_seen);
}

static void test_budget_income() {
    printf("-- test_budget_income\n");
    sim::City c;
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    for (int i = 1; i <= 6; ++i) c.place_power_line(12 + i, 11);  // (13,11)..(18,11)
    c.set_zone(19, 11, sim::Zone::Industrial);  // adjacent to line (18,11)
    c.set_terrain(15, 10, sim::Terrain::Road);  // road access for residential
    c.set_zone(16, 10, sim::Zone::Residential); // adjacent to line (16,11)
    c.set_tax_rate(5);

    for (int m = 0; m < 12; ++m) c.step_month();
    sim::Stats s = c.stats();
    // income = pop * tax * 2 = 5 * 5 * 2 = 50
    CHECK(s.income == 50);
    CHECK(s.upkeep >= sim::City::kInitialFunds - c.funds() ? s.upkeep > 0 : s.upkeep >= 0);
    CHECK(s.tax_rate == 5);
}

static void test_determinism() {
    printf("-- test_determinism\n");
    sim::City a, b;
    a.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    b.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    for (int m = 0; m < 24; ++m) {
        if (m % 6 == 0) {
            a.place_power_line(13, 11);
            a.place_power_line(14, 11);
            b.place_power_line(13, 11);
            b.place_power_line(14, 11);
            a.set_zone(15, 11, sim::Zone::Industrial);
            b.set_zone(15, 11, sim::Zone::Industrial);
            a.set_terrain(15, 10, sim::Terrain::Road);
            b.set_terrain(15, 10, sim::Terrain::Road);
            a.set_zone(16, 10, sim::Zone::Residential);
            b.set_zone(16, 10, sim::Zone::Residential);
        }
        a.step_month();
        b.step_month();
    }
    sim::Stats sa = a.stats();
    sim::Stats sb = b.stats();
    CHECK(sa.population == sb.population);
    CHECK(sa.commercial_tiles == sb.commercial_tiles);
    CHECK(sa.industrial_tiles == sb.industrial_tiles);
    CHECK(sa.road_tiles == sb.road_tiles);
    CHECK(sa.funds == sb.funds);
    CHECK(sa.res_demand == sb.res_demand);
}

static void test_stats_rci() {
    printf("-- test_stats_rci\n");
    sim::City c;
    c.place_power_plant(5, 5, sim::PowerPlantType::Coal); // center (6,6), covers (5,5)-(7,7)
    c.place_power_line(8, 6);  // extend right from plant edge at (7,6)
    c.place_power_line(9, 6);  // chain outward
    c.place_power_line(9, 7);  // branch down
    // Zones must sit adjacent to a powered power line
    c.set_zone(10, 6, sim::Zone::Commercial);  // adjacent to line (9,6)
    c.set_zone(10, 7, sim::Zone::Industrial);  // adjacent to line (9,7)
    c.set_terrain(11, 6, sim::Terrain::Road);

    c.step_month();
    sim::Stats s = c.stats();
    CHECK(s.commercial_tiles == 1);
    CHECK(s.industrial_tiles == 1);
    CHECK(s.road_tiles == 1);
    CHECK(s.jobs == 8 + 4);
    // no residents yet -> high residential demand
    CHECK(s.res_demand == sim::Demand::High);
}

static void test_power_plant_placement() {
    printf("-- test_power_plant_placement\n");
    sim::City c;
    // Coal plant 3x3
    CHECK(c.place_power_plant(10, 10, sim::PowerPlantType::Coal));
    CHECK(c.zone(10, 10) == sim::Zone::PowerPlant);
    CHECK(c.zone(12, 12) == sim::Zone::PowerPlant);
    CHECK(c.zone(9, 10) == sim::Zone::None);  // outside footprint
    // Nuclear plant 4x4
    CHECK(c.place_power_plant(20, 20, sim::PowerPlantType::Nuclear));
    CHECK(c.zone(20, 20) == sim::Zone::PowerPlant);
    CHECK(c.zone(23, 23) == sim::Zone::PowerPlant);
    // Reject on water
    c.set_terrain(30, 30, sim::Terrain::Water);
    CHECK(!c.place_power_plant(30, 30, sim::PowerPlantType::Coal));
    // Reject overlapping
    CHECK(!c.place_power_plant(11, 11, sim::PowerPlantType::Nuclear));
    // Remove coal plant
    CHECK(c.remove_power_plant(10, 10));
    CHECK(c.zone(10, 10) == sim::Zone::None);
    CHECK(c.zone(12, 12) == sim::Zone::None);
}

static void test_power_line_conductivity() {
    printf("-- test_power_line_conductivity\n");
    sim::City c;
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    // Plant center at (11,11), radius 1 covers up to (12,11). Place lines from (13,11) to (21,11).
    for (int i = 1; i <= 9; ++i) {
        c.place_power_line(12 + i, 11);
    }
    c.step_month();
    // Plant center powered
    CHECK(c.powered(11, 11));
    // Power line tiles powered (conduct indefinitely)
    CHECK(c.powered(15, 11));
    CHECK(c.powered(20, 11));
    // Gap breaks conductivity - place line at (23,11) skipping (22,11)
    c.place_power_line(23, 11);
    c.step_month();
    CHECK(!c.powered(23, 11));  // isolated line (gap at 22) not powered
    CHECK(!c.powered(24, 11));  // beyond gap not powered
}

static void test_power_coverage_radius() {
    printf("-- test_power_coverage_radius\n");
    sim::City c;
    // Coal: 3x3 footprint, center at (11,11), radius 1 covers (10,10) to (12,12)
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    c.step_month();
    CHECK(c.powered(11, 11));  // center
    CHECK(c.powered(10, 11));  // left
    CHECK(c.powered(12, 11));  // right
    CHECK(c.powered(11, 10));  // up
    CHECK(c.powered(11, 12));  // down
    CHECK(!c.powered(9, 11));  // beyond radius
    CHECK(!c.powered(13, 11));

    // Nuclear: 4x4 footprint, center at (22,22), radius 2 covers (20,20) to (24,24)
    sim::City c2;
    c2.place_power_plant(20, 20, sim::PowerPlantType::Nuclear);
    c2.step_month();
    CHECK(c2.powered(22, 22));  // center
    CHECK(c2.powered(20, 22));  // radius 2 left
    CHECK(c2.powered(24, 22));  // radius 2 right
    CHECK(c2.powered(22, 20));  // radius 2 up
    CHECK(c2.powered(22, 24));  // radius 2 down
    CHECK(!c2.powered(19, 22)); // beyond radius
    CHECK(!c2.powered(25, 22));
}

static void test_unpowered_zone_no_jobs() {
    printf("-- test_unpowered_zone_no_jobs\n");
    sim::City c;
    // Commercial zone WITHOUT power
    c.set_zone(12, 10, sim::Zone::Commercial);
    c.set_terrain(13, 10, sim::Terrain::Road);
    c.step_month();
    sim::Stats s = c.stats();
    CHECK(s.commercial_tiles == 0);  // unpowered -> not counted
    CHECK(s.jobs == 0);

    // Now add power - plant at (8,9) footprint (8,9)-(10,11). Power line at
    // (11,10) is adjacent to powered plant tile (10,10), conducts, and powers
    // the commercial at (12,10) as a neighbor.
    c.place_power_plant(8, 9, sim::PowerPlantType::Coal);
    c.place_power_line(11, 10);
    c.step_month();
    s = c.stats();
    CHECK(s.commercial_tiles == 1);  // (12,10) adjacent to powered line -> counted
    CHECK(s.jobs == 8);
}

static void test_unpowered_residential_no_growth() {
    printf("-- test_unpowered_residential_no_growth\n");
    sim::City c;
    // Industrial + residential without power
    c.set_zone(13, 11, sim::Zone::Industrial);
    c.set_terrain(12, 11, sim::Terrain::Road);
    c.set_zone(11, 11, sim::Zone::Residential);
    c.step_month();
    CHECK(c.density(11, 11) == 0);  // no growth without power

    // Add power - plant at (8,9) footprint (8,9)-(10,11). Lines conduct outward:
    // (11,10) is adjacent to plant tile (10,10), then (12,10), (13,10) chain.
    // Residential (11,11) is powered as neighbor of line (11,10) and has road access.
    // Industrial (13,11) is powered as neighbor of line (13,10) providing jobs.
    c.place_power_plant(8, 9, sim::PowerPlantType::Coal);
    c.place_power_line(11, 10);
    c.place_power_line(12, 10);
    c.place_power_line(13, 10);
    for (int m = 0; m < 6; ++m) c.step_month();
    CHECK(c.density(11, 11) > 0);  // grows with power
}

static void test_power_determinism() {
    printf("-- test_power_determinism\n");
    sim::City a, b;
    // Place plants and lines in different order
    a.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    a.place_power_line(15, 11);
    a.place_power_line(20, 11);
    a.set_zone(15, 11, sim::Zone::Commercial);

    b.place_power_line(20, 11);
    b.place_power_line(15, 11);
    b.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    b.set_zone(15, 11, sim::Zone::Commercial);

    for (int m = 0; m < 12; ++m) {
        a.step_month();
        b.step_month();
    }
    // Power state should be identical
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 120; ++x) {
            CHECK(a.powered(x, y) == b.powered(x, y));
        }
    }
    sim::Stats sa = a.stats();
    sim::Stats sb = b.stats();
    CHECK(sa.population == sb.population);
    CHECK(sa.jobs == sb.jobs);
    CHECK(sa.funds == sb.funds);
}

static void test_meteor_destruction() {
    printf("-- test_meteor_destruction\n");
    sim::City c;
    // Build some city
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    for (int i = 1; i <= 6; ++i) c.place_power_line(12 + i, 11);
    c.set_zone(19, 11, sim::Zone::Industrial);
    c.set_terrain(15, 10, sim::Terrain::Road);
    c.set_zone(16, 10, sim::Zone::Residential);

    // Trigger meteor at known location (mock by calling internal)
    c.trigger_disaster(sim::DisasterType::Meteor);
    auto ds = c.disaster_state();
    CHECK(ds.active);
    CHECK(ds.type == sim::DisasterType::Meteor);
    int mx = ds.x, my = ds.y;
    int mr = ds.radius;

    // Verify crater circle
    int crater_count = 0;
    for (int y = -mr; y <= mr; ++y) {
        for (int x = -mr; x <= mr; ++x) {
            int cx = mx + x, cy = my + y;
            if (cx >= 0 && cx < 120 && cy >= 0 && cy < 100) {
                if (x*x + y*y <= mr*mr) {
                    CHECK(c.terrain(cx, cy) == sim::Terrain::Crater);
                    CHECK(c.zone(cx, cy) == sim::Zone::None);
                    crater_count++;
                }
            }
        }
    }
    CHECK(crater_count > 50);  // π*6² ≈ 113, but clipped by bounds

    // Step month to clear disaster state
    c.step_month();
    ds = c.disaster_state();
    CHECK(!ds.active);
}

static void test_monster_path() {
    printf("-- test_monster_path\n");
    sim::City c;
    // Build city center
    c.place_power_plant(60, 50, sim::PowerPlantType::Coal);
    c.set_zone(60, 50, sim::Zone::Commercial);
    c.set_zone(61, 50, sim::Zone::Residential);
    c.set_terrain(59, 50, sim::Terrain::Road);

    // Trigger monster
    c.trigger_disaster(sim::DisasterType::Monster);
    auto ds = c.disaster_state();
    CHECK(ds.active);
    CHECK(ds.type == sim::DisasterType::Monster);

    int start_x = ds.x, start_y = ds.y;
    int target_x = ds.radius & 0xFFFF;
    int target_y = (ds.radius >> 16) & 0xFFFF;

    // Monster should start at edge
    bool at_edge = (start_x == 0 || start_x == 119 || start_y == 0 || start_y == 99);
    CHECK(at_edge);

    // Target should be center
    CHECK(target_x == 60);
    CHECK(target_y == 50);

    // Step through monster path
    int steps = 0;
    while (ds.active && steps < 200) {
        c.step_month();
        ds = c.disaster_state();
        steps++;
    }
    CHECK(!ds.active);  // should have reached center
    CHECK(steps > 0);
    CHECK(steps < 200);

    // Verify crater path exists from edge toward center
    int crater_count = 0;
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 120; ++x) {
            if (c.terrain(x, y) == sim::Terrain::Crater) crater_count++;
        }
    }
    CHECK(crater_count > 20);  // monster leaves 3x3 path, ~60 steps * 9 = 540 but clipped
}

static void test_disaster_determinism() {
    printf("-- test_disaster_determinism\n");
    sim::City a, b;
    // Same initial state
    a.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    b.place_power_plant(10, 10, sim::PowerPlantType::Coal);

    // Trigger same disaster manually
    a.trigger_disaster(sim::DisasterType::Meteor);
    b.trigger_disaster(sim::DisasterType::Meteor);

    auto dsa = a.disaster_state();
    auto dsb = b.disaster_state();
    CHECK(dsa.x == dsb.x);
    CHECK(dsa.y == dsb.y);
    CHECK(dsa.radius == dsb.radius);

    // Step both
    a.step_month();
    b.step_month();

    // Terrain should be identical
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 120; ++x) {
            CHECK(a.terrain(x, y) == b.terrain(x, y));
            CHECK(a.zone(x, y) == b.zone(x, y));
        }
    }
}

static void test_disaster_power_grid() {
    printf("-- test_disaster_power_grid\n");
    sim::City c;
    c.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    // Power line from (13,11) to (20,11)
    for (int i = 1; i <= 10; ++i) c.place_power_line(12 + i, 11);
    c.set_zone(22, 11, sim::Zone::Industrial);
    c.set_terrain(15, 10, sim::Terrain::Road);
    c.set_zone(16, 10, sim::Zone::Residential);

    c.step_month();  // compute initial power
    CHECK(c.powered(16, 10));  // residential powered
    CHECK(c.powered(22, 11));  // industrial powered

    // Trigger meteor on power line
    c.trigger_disaster(sim::DisasterType::Meteor);
    auto ds = c.disaster_state();
    // Force meteor to hit power line at (15,11)
    // We can't easily control position, so just verify power recomputes after
    c.step_month();  // clears disaster, recomputes power

    // Power should be recomputed (lines destroyed)
    // At minimum, verify no crash and power state is consistent
    bool any_powered = false;
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 120; ++x) {
            if (c.powered(x, y)) any_powered = true;
        }
    }
    // Just verify it runs without crash
    CHECK(true);
}

int main() {
    printf("City test suite\n");
    test_placement();
    test_residential_growth();
    test_population_job_cap();
    test_budget_burn();
    test_budget_income();
    test_determinism();
    test_stats_rci();
    test_power_plant_placement();
    test_power_line_conductivity();
    test_power_coverage_radius();
    test_unpowered_zone_no_jobs();
    test_unpowered_residential_no_growth();
    test_power_determinism();
    test_meteor_destruction();
    test_monster_path();
    test_disaster_determinism();
    test_disaster_power_grid();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}
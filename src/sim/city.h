#pragma once

#include <cstdint>

namespace sim {

static constexpr int kMapWidth  = 120;
static constexpr int kMapHeight = 100;

enum class Terrain : uint8_t {
    Grass      = 0,
    Water      = 1,
    Tree       = 2,
    Road       = 3,
    PowerLine  = 4,
    Crater     = 5
};

enum class Zone : uint8_t {
    None        = 0,
    Residential = 1,
    Commercial  = 2,
    Industrial  = 3,
    PowerPlant  = 4
};

enum class PowerPlantType : uint8_t {
    None    = 0,
    Coal    = 1,
    Nuclear = 2
};

enum class Demand { Low, Medium, High };

enum class DisasterType { None, Meteor, Monster, Earthquake };

struct DisasterState {
    DisasterType type = DisasterType::None;
    bool active = false;
    int timer = 0;
    int x = 0, y = 0;
    int radius = 0;
};

struct Stats {
    int population  = 0;
    int workforce   = 0;      // available workers (commercial jobs)
    int jobs        = 0;      // total jobs (commercial + industrial)
    int resident_tiles = 0;
    int commercial_tiles = 0;
    int industrial_tiles = 0;
    int road_tiles  = 0;
    int64_t funds   = 0;
    int tax_rate    = 0;
    int64_t income  = 0;      // last month
    int64_t upkeep  = 0;      // last month
    Demand res_demand;
    Demand com_demand;
    Demand ind_demand;
};

// Public read-only view of a single map cell.
struct TileView {
    Terrain terrain;
    Zone    zone;
    int     density;
};

// Native reimplementation of the SimCity SNES simulation model. No dependency
// on SDL or ROM data. Deterministic: identical operation sequences produce
// identical states.
class City {
public:
    City();

    Terrain terrain(int x, int y) const;
    void    set_terrain(int x, int y, Terrain t);

    Zone   zone(int x, int y) const;
    void   set_zone(int x, int y, Zone z);

    // Residential density for a cell (0..kMaxDensity).
    int    density(int x, int y) const;

    TileView tile(int x, int y) const;

    // Advances one simulated month.
    void step_month();

    // Pending-value setters (used by the UI before step_month).
    void set_tax_rate(int rate);   // percent, 0..20
    int  tax_rate() const { return tax_rate_; }

    int64_t funds() const { return funds_; }
    void set_funds(int64_t f) { funds_ = f; }
    Stats   stats() const;

    // Apply a 120x100 tile map from ROM scenario decoder.
    // tiles_120x100: row-major, low 10 bits = SNES tile char.
    void apply_terrain_map(const uint16_t* tiles_120x100);

    // Power grid API
    bool powered(int x, int y) const;

    bool place_power_plant(int x, int y, PowerPlantType type);
    bool remove_power_plant(int x, int y);

    bool place_power_line(int x, int y);
    bool remove_power_line(int x, int y);

    // Disaster API
    DisasterState disaster_state() const { return disaster_; }
    void trigger_disaster(DisasterType type);  // manual trigger for testing

    static constexpr int kMaxDensity = 5;
    static constexpr int kInitialFunds = 20000;

private:
    bool in_bounds(int x, int y) const {
        return x >= 0 && x < kMapWidth && y >= 0 && y < kMapHeight;
    }

    struct Tile {
        Terrain          terrain     = Terrain::Grass;
        Zone             zone        = Zone::None;
        uint8_t          density     = 0;
        bool             powered     = false;
        PowerPlantType   plant_type  = PowerPlantType::None;
    };

    bool   road_adjacent(int x, int y) const;
    int    demand_index() const;

    // Power grid internals
    void   compute_power();
    int    power_radius(PowerPlantType type) const;
    bool   is_power_line(int x, int y) const;
    void   flood_fill_power(int px, int py, int radius);

    // Disaster internals
    void   process_disasters();
    void   trigger_meteor();
    void   trigger_monster();
    void   trigger_earthquake();
    void   update_monster();
    uint32_t next_random();

    Tile    tiles_[kMapHeight][kMapWidth];
    int     tax_rate_ = 5;
    int64_t funds_    = kInitialFunds;
    int64_t last_income_ = 0;
    int64_t last_upkeep_ = 0;
    uint32_t month_      = 0;
    DisasterState disaster_;
    uint32_t rng_state_  = 0;
};

} // namespace sim
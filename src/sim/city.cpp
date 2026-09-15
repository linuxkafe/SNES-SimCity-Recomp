#include "sim/city.h"
#include <cstdio>

namespace sim {

namespace {

// Budget tuning constants (monthly, per unit).
constexpr int   kComJobsPerTile  = 8;
constexpr int   kIndJobsPerTile  = 4;
constexpr int   kJobCapacityFactor = 2;   // residents per job before res demand drops
constexpr int64_t kIncomePerPopTax = 2;   // dollars per resident per tax %

constexpr int64_t kUpkeepRes       = 1;
constexpr int64_t kUpkeepCom       = 2;
constexpr int64_t kUpkeepInd       = 2;
constexpr int64_t kUpkeepRoad      = 1;
constexpr int64_t kUpkeepPowerLine = 1;
constexpr int64_t kUpkeepCoalPlant = 50;
constexpr int64_t kUpkeepNuclearPlant = 100;
constexpr int64_t kCountyUpkeep    = 500;    // base county cost every month

Demand demand_of(int numerator, int denominator) {
    if (denominator == 0) return numerator > 0 ? Demand::High : Demand::Low;
    if (numerator >= 2 * denominator) return Demand::High;
    if (numerator >= denominator)     return Demand::Medium;
    return Demand::Low;
}

} // namespace

City::City() : rng_state_(0xACE1u) {}

Terrain City::terrain(int x, int y) const {
    if (!in_bounds(x, y)) return Terrain::Grass;
    return tiles_[y][x].terrain;
}

void City::set_terrain(int x, int y, Terrain t) {
    if (!in_bounds(x, y)) return;
    tiles_[y][x].terrain = t;
    tiles_[y][x].zone    = Zone::None;  // bulldoze
    tiles_[y][x].density = 0;
}

Zone City::zone(int x, int y) const {
    if (!in_bounds(x, y)) return Zone::None;
    return tiles_[y][x].zone;
}

void City::set_zone(int x, int y, Zone z) {
    if (!in_bounds(x, y)) return;
    if (tiles_[y][x].terrain == Terrain::Water) return;  // can't zone water
    tiles_[y][x].zone    = z;
    tiles_[y][x].density = 0;
}

int City::density(int x, int y) const {
    if (!in_bounds(x, y)) return 0;
    return tiles_[y][x].density;
}

TileView City::tile(int x, int y) const {
    if (!in_bounds(x, y)) return {Terrain::Grass, Zone::None, 0};
    const Tile& t = tiles_[y][x];
    return {t.terrain, t.zone, t.density};
}

bool City::road_adjacent(int x, int y) const {
    return terrain(x - 1, y) == Terrain::Road ||
           terrain(x + 1, y) == Terrain::Road ||
           terrain(x, y - 1) == Terrain::Road ||
           terrain(x, y + 1) == Terrain::Road;
}

void City::set_tax_rate(int rate) {
    if (rate < 0) rate = 0;
    if (rate > 20) rate = 20;
    tax_rate_ = rate;
}

void City::step_month() {
    ++month_;

    // Disasters: process at start of month
    process_disasters();

    // Power grid: compute coverage first
    compute_power();

    // Aggregate city state for this month (only powered zones count).
    int resident_tiles = 0;
    int commercial_tiles = 0;
    int industrial_tiles = 0;
    int power_plant_tiles = 0;
    int power_line_tiles = 0;
    int road_tiles = 0;
    int population = 0;
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            const Tile& t = tiles_[y][x];
            if (t.terrain == Terrain::Road) ++road_tiles;
            if (t.terrain == Terrain::PowerLine) ++power_line_tiles;
            if (t.zone == Zone::PowerPlant) ++power_plant_tiles;
            if (!t.powered) continue;  // unpowered zones don't count
            switch (t.zone) {
                case Zone::Residential: ++resident_tiles; population += t.density; break;
                case Zone::Commercial:  ++commercial_tiles; break;
                case Zone::Industrial:  ++industrial_tiles; break;
                default: break;
            }
        }
    }

    // Jobs only from powered commercial/industrial
    const int jobs = commercial_tiles * kComJobsPerTile +
                     industrial_tiles * kIndJobsPerTile;

    // RCI demand.
    const Demand res_demand = demand_of(jobs * kJobCapacityFactor - population, 1);

    // Residential density moves toward jobs capacity (only if powered).
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            Tile& t = tiles_[y][x];
            if (t.zone != Zone::Residential) continue;
            if (!t.powered) continue;  // unpowered residential stalls

            const bool road = road_adjacent(x, y);
            if (t.density < kMaxDensity && res_demand != Demand::Low && funds_ > 0) {
                // Road-connected lots grow every month; landlocked growth is
                // limited to strong-demand odd months (deterministic).
                if (road || (res_demand == Demand::High && (month_ & 1))) {
                    ++t.density;
                }
            } else if (t.density > 0 && res_demand == Demand::Low && month_ % 3 == 0) {
                --t.density;  // abandonment
            }
        }
    }

    // Recompute population after growth.
    population = 0;
    for (int y = 0; y < kMapHeight; ++y)
        for (int x = 0; x < kMapWidth; ++x)
            if (tiles_[y][x].zone == Zone::Residential && tiles_[y][x].powered)
                population += tiles_[y][x].density;

    // Budget.
    int64_t income = static_cast<int64_t>(population) * tax_rate_ * kIncomePerPopTax;
    int64_t upkeep = kCountyUpkeep +
                     resident_tiles * kUpkeepRes +
                     commercial_tiles * kUpkeepCom +
                     industrial_tiles * kUpkeepInd +
                     road_tiles * kUpkeepRoad +
                     power_line_tiles * kUpkeepPowerLine +
                     power_plant_tiles * kUpkeepCoalPlant;  // simplified: coal cost for all
    last_income_ = income;
    last_upkeep_ = upkeep;
    funds_ += income - upkeep;
    if (funds_ < 0) funds_ = 0;
}

Stats City::stats() const {
    Stats s;
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            const Tile& t = tiles_[y][x];
            if (t.terrain == Terrain::Road) ++s.road_tiles;
            if (!t.powered) continue;  // unpowered zones don't count in stats
            switch (t.zone) {
                case Zone::Residential:
                    ++s.resident_tiles;
                    s.population += t.density;
                    break;
                case Zone::Commercial: ++s.commercial_tiles; break;
                case Zone::Industrial: ++s.industrial_tiles; break;
                default: break;
            }
        }
    }
    s.jobs = s.commercial_tiles * kComJobsPerTile +
             s.industrial_tiles * kIndJobsPerTile;
    s.workforce = s.resident_tiles * kMaxDensity;
    s.funds = funds_;
    s.tax_rate = tax_rate_;
    s.income = last_income_;
    s.upkeep = last_upkeep_;
    s.res_demand = demand_of(s.jobs * kJobCapacityFactor - s.population, 1);
    s.com_demand = demand_of(s.population / 2, s.commercial_tiles);
    s.ind_demand = demand_of(s.population / 2, s.industrial_tiles);
    return s;
}

namespace {
    const Terrain* get_tile_to_terrain_table() {
        static Terrain table[1024];
        static bool initialized = false;
        if (!initialized) {
            for (int i = 0; i < 1024; ++i) table[i] = Terrain::Grass;
            table[0] = Terrain::Grass;
            table[1] = table[2] = table[3] = Terrain::Water;
            for (int i = 4; i <= 19; ++i) table[i] = Terrain::Grass;
            for (int i = 20; i <= 37; ++i) table[i] = Terrain::Tree;
            table[38] = table[39] = Terrain::Grass;
            table[40] = table[41] = Terrain::Grass;
            for (int i = 48; i <= 95; ++i) table[i] = Terrain::Road;
            for (int i = 112; i <= 127; ++i) table[i] = Terrain::Grass;
            initialized = true;
        }
        return table;
    }
}

void City::apply_terrain_map(const uint16_t* tiles_120x100) {
    const Terrain* kTileToTerrain = get_tile_to_terrain_table();
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            uint16_t tile_id = tiles_120x100[y * kMapWidth + x] & 0x03FF;
            Terrain t = (tile_id < 1024) ? kTileToTerrain[tile_id] : Terrain::Grass;
            tiles_[y][x].terrain = t;
            tiles_[y][x].zone = Zone::None;
            tiles_[y][x].density = 0;
            tiles_[y][x].powered = false;
            tiles_[y][x].plant_type = PowerPlantType::None;
        }
    }
}

bool City::powered(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    return tiles_[y][x].powered;
}

bool City::place_power_plant(int x, int y, PowerPlantType type) {
    if (!in_bounds(x, y)) return false;
    if (type == PowerPlantType::None) return false;

    int radius = power_radius(type);
    int footprint = (type == PowerPlantType::Coal) ? 3 : 4;

    // Check footprint is clear (no water, no existing zones, in bounds)
    for (int dy = 0; dy < footprint; ++dy) {
        for (int dx = 0; dx < footprint; ++dx) {
            int tx = x + dx;
            int ty = y + dy;
            if (!in_bounds(tx, ty)) return false;
            if (tiles_[ty][tx].terrain == Terrain::Water) return false;
            if (tiles_[ty][tx].zone != Zone::None) return false;
        }
    }

    // Place plant
    for (int dy = 0; dy < footprint; ++dy) {
        for (int dx = 0; dx < footprint; ++dx) {
            int tx = x + dx;
            int ty = y + dy;
            tiles_[ty][tx].zone = Zone::PowerPlant;
            tiles_[ty][tx].plant_type = type;
            tiles_[ty][tx].terrain = Terrain::Grass;  // plant sits on grass
            tiles_[ty][tx].density = 0;
        }
    }
    return true;
}

bool City::remove_power_plant(int x, int y) {
    if (!in_bounds(x, y)) return false;
    if (tiles_[y][x].zone != Zone::PowerPlant) return false;

    PowerPlantType type = tiles_[y][x].plant_type;
    int footprint = (type == PowerPlantType::Coal) ? 3 : 4;

    // Find top-left of plant footprint
    int start_x = x, start_y = y;
    while (start_x > 0 && tiles_[y][start_x - 1].zone == Zone::PowerPlant &&
           tiles_[y][start_x - 1].plant_type == type) --start_x;
    while (start_y > 0 && tiles_[start_y - 1][x].zone == Zone::PowerPlant &&
           tiles_[start_y - 1][x].plant_type == type) --start_y;

    // Clear footprint
    for (int dy = 0; dy < footprint; ++dy) {
        for (int dx = 0; dx < footprint; ++dx) {
            int tx = start_x + dx;
            int ty = start_y + dy;
            if (in_bounds(tx, ty) &&
                tiles_[ty][tx].zone == Zone::PowerPlant &&
                tiles_[ty][tx].plant_type == type) {
                tiles_[ty][tx].zone = Zone::None;
                tiles_[ty][tx].plant_type = PowerPlantType::None;
                tiles_[ty][tx].terrain = Terrain::Grass;
                tiles_[ty][tx].density = 0;
            }
        }
    }
    return true;
}

bool City::place_power_line(int x, int y) {
    if (!in_bounds(x, y)) return false;
    if (tiles_[y][x].terrain == Terrain::Water) return false;
    if (tiles_[y][x].zone != Zone::None) return false;  // can't place on zoned land
    tiles_[y][x].terrain = Terrain::PowerLine;
    tiles_[y][x].zone = Zone::None;
    tiles_[y][x].density = 0;
    return true;
}

bool City::remove_power_line(int x, int y) {
    if (!in_bounds(x, y)) return false;
    if (tiles_[y][x].terrain != Terrain::PowerLine) return false;
    tiles_[y][x].terrain = Terrain::Grass;
    return true;
}

int City::power_radius(PowerPlantType type) const {
    switch (type) {
        case PowerPlantType::Coal:    return 1;  // 3x3 centered = radius 1
        case PowerPlantType::Nuclear: return 2;  // 4x4 centered = radius 2
        default: return 0;
    }
}

bool City::is_power_line(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    return tiles_[y][x].terrain == Terrain::PowerLine;
}

void City::flood_fill_power(int px, int py, int radius) {
    // BFS queue for power line conduction
    struct QueueEntry { int x, y; };
    static QueueEntry queue[12000];
    int qhead = 0, qtail = 0;

    // Visited array for power line conduction
    static bool visited[100][120];
    for (int yy = 0; yy < kMapHeight; ++yy)
        for (int xx = 0; xx < kMapWidth; ++xx)
            visited[yy][xx] = false;

    // Deterministic neighbor order: up, right, down, left
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};

    // Step 1: Power all tiles within plant radius (Chebyshev/square distance)
    // Also collect power lines within or adjacent to radius for conduction
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            int dx = x - px;
            int dy = y - py;
            int adx = dx >= 0 ? dx : -dx;
            int ady = dy >= 0 ? dy : -dy;
            int chebyshev_dist = adx > ady ? adx : ady;
            if (chebyshev_dist <= radius) {
                tiles_[y][x].powered = true;
                // If this tile is a power line, add to queue for conduction
                if (is_power_line(x, y) && !visited[y][x]) {
                    visited[y][x] = true;
                    queue[qtail++] = {x, y};
                }
            }
        }
    }

    // Step 1b: Power lines adjacent to ANY powered tile connect to the grid
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            if (tiles_[y][x].powered) {
                // Check 4 neighbors for power lines
                for (int dir = 0; dir < 4; ++dir) {
                    int nx = x + dx[dir];
                    int ny = y + dy[dir];
                    if (in_bounds(nx, ny) && is_power_line(nx, ny) && !visited[ny][nx]) {
                        visited[ny][nx] = true;
                        tiles_[ny][nx].powered = true;
                        queue[qtail++] = {nx, ny};
                    }
                }
            }
        }
    }

    // Step 2: Flood fill through power lines (indefinite conduction)
    while (qhead < qtail) {
        QueueEntry cur = queue[qhead++];
        int cx = cur.x, cy = cur.y;

        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];

            if (!in_bounds(nx, ny)) continue;
            if (visited[ny][nx]) continue;

            // Power lines conduct indefinitely
            if (is_power_line(nx, ny)) {
                visited[ny][nx] = true;
                tiles_[ny][nx].powered = true;
                queue[qtail++] = {nx, ny};
            }
        }
    }

    // Step 3: Power all tiles adjacent to powered power lines (zones, roads, etc.)
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            if (tiles_[y][x].powered && is_power_line(x, y)) {
                // Power all 4 neighbors of this power line
                for (int dir = 0; dir < 4; ++dir) {
                    int nx = x + dx[dir];
                    int ny = y + dy[dir];
                    if (in_bounds(nx, ny)) {
                        tiles_[ny][nx].powered = true;
                    }
                }
            }
        }
    }
}

void City::compute_power() {
    // 1. Clear all powered flags
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            tiles_[y][x].powered = false;
        }
    }

    // 2. Collect all power plant centers (deterministic order: y then x)
    // Use center of plant footprint for flood-fill origin
    struct Plant { int x, y; PowerPlantType type; int radius; };
    Plant plants[200];
    int plant_count = 0;

    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            if (tiles_[y][x].zone == Zone::PowerPlant) {
                // Only process top-left tile of each plant footprint
                bool is_top_left = true;
                if (x > 0 && tiles_[y][x-1].zone == Zone::PowerPlant &&
                    tiles_[y][x-1].plant_type == tiles_[y][x].plant_type) is_top_left = false;
                if (y > 0 && tiles_[y-1][x].zone == Zone::PowerPlant &&
                    tiles_[y-1][x].plant_type == tiles_[y][x].plant_type) is_top_left = false;

                if (is_top_left) {
                    PowerPlantType type = tiles_[y][x].plant_type;
                    int footprint = (type == PowerPlantType::Coal) ? 3 : 4;
                    // Flood from center of footprint
                    int center_x = x + footprint / 2;
                    int center_y = y + footprint / 2;
                    plants[plant_count++] = {center_x, center_y, type,
                                             power_radius(type)};
                }
            }
        }
    }

    // 3. Flood-fill from each plant
    for (int p = 0; p < plant_count; ++p) {
        flood_fill_power(plants[p].x, plants[p].y, plants[p].radius);
    }
}

uint32_t City::next_random() {
    rng_state_ = rng_state_ * 1664525u + 1013904223u;
    return rng_state_;
}

void City::trigger_disaster(DisasterType type) {
    if (type == DisasterType::Meteor) {
        trigger_meteor();
    } else if (type == DisasterType::Monster) {
        trigger_monster();
    } else if (type == DisasterType::Earthquake) {
        trigger_earthquake();
    }
}

void City::process_disasters() {
    if (!disaster_.active) {
        // Monthly disaster roll: base 1% chance, capped at 5% (T024)
        // Original SNES disasters are rare events, not monthly occurrences.
        uint32_t roll = next_random() % 10000;
        int threshold = 100;  // 1% base chance
        if (threshold > 500) threshold = 500;  // cap at 5%
        if (roll < static_cast<uint32_t>(threshold)) {
            // Randomly pick meteor, monster, or earthquake (T024)
            int pick = next_random() % 3;
            if (pick == 0) {
                trigger_meteor();
            } else if (pick == 1) {
                trigger_monster();
            } else {
                trigger_earthquake();
            }
        }
        return;
    }

    // Active disaster: update
    if (disaster_.type == DisasterType::Meteor) {
        // Meteor is instant; just clear after 1 month for display
        disaster_.timer++;
        if (disaster_.timer >= 1) {
            disaster_.active = false;
            disaster_.type = DisasterType::None;
        }
    } else if (disaster_.type == DisasterType::Earthquake) {
        // Earthquake is instant; clear after 1 month for display
        disaster_.timer++;
        if (disaster_.timer >= 1) {
            disaster_.active = false;
            disaster_.type = DisasterType::None;
        }
    } else if (disaster_.type == DisasterType::Monster) {
        update_monster();
    }
}

void City::trigger_meteor() {
    // Pick random valid coordinate (not water)
    int attempts = 0;
    int tx = 0, ty = 0;
    do {
        tx = static_cast<int>(next_random() % kMapWidth);
        ty = static_cast<int>(next_random() % kMapHeight);
        attempts++;
    } while (attempts < 100 && tiles_[ty][tx].terrain == Terrain::Water);

    int radius = 6;  // ~6 tile radius crater

    // Destroy terrain/zones in circle
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            int cx = tx + x;
            int cy = ty + y;
            if (!in_bounds(cx, cy)) continue;
            int dx = x, dy = y;
            if (dx*dx + dy*dy <= radius*radius) {
                tiles_[cy][cx].terrain = Terrain::Crater;
                tiles_[cy][cx].zone = Zone::None;
                tiles_[cy][cx].density = 0;
                tiles_[cy][cx].plant_type = PowerPlantType::None;
            }
        }
    }

    disaster_ = {DisasterType::Meteor, true, 0, tx, ty, radius};
}

void City::trigger_monster() {
    // Spawn at random map edge
    int edge = next_random() % 4;
    int tx = 0, ty = 0;
    int target_x = kMapWidth / 2;
    int target_y = kMapHeight / 2;

    switch (edge) {
        case 0: // top
            tx = static_cast<int>(next_random() % kMapWidth);
            ty = 0;
            break;
        case 1: // right
            tx = kMapWidth - 1;
            ty = static_cast<int>(next_random() % kMapHeight);
            break;
        case 2: // bottom
            tx = static_cast<int>(next_random() % kMapWidth);
            ty = kMapHeight - 1;
            break;
        case 3: // left
            tx = 0;
            ty = static_cast<int>(next_random() % kMapHeight);
            break;
    }

    disaster_ = {DisasterType::Monster, true, 0, tx, ty, 0};
    // Store target in radius field (repurposed)
    disaster_.radius = target_x | (target_y << 16);
}

void City::trigger_earthquake() {
    // Pick random valid coordinate (not water)
    int attempts = 0;
    int tx = 0, ty = 0;
    do {
        tx = static_cast<int>(next_random() % kMapWidth);
        ty = static_cast<int>(next_random() % kMapHeight);
        attempts++;
    } while (attempts < 100 && tiles_[ty][tx].terrain == Terrain::Water);

    int radius = 8;  // Earthquake affects larger area than meteor

    // Crack roads and damage buildings in radius
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            int cx = tx + x;
            int cy = ty + y;
            if (!in_bounds(cx, cy)) continue;
            int dx = x, dy = y;
            if (dx*dx + dy*dy <= radius*radius) {
                // Crack roads -> Crater
                if (tiles_[cy][cx].terrain == Terrain::Road) {
                    tiles_[cy][cx].terrain = Terrain::Crater;
                }
                // Damage buildings (reduce density)
                if (tiles_[cy][cx].zone != Zone::None &&
                    tiles_[cy][cx].zone != Zone::PowerPlant) {
                    if (tiles_[cy][cx].density > 0) {
                        tiles_[cy][cx].density = tiles_[cy][cx].density > 1
                            ? tiles_[cy][cx].density - 1 : 0;
                    }
                }
                // Power lines -> Crater
                if (tiles_[cy][cx].terrain == Terrain::PowerLine) {
                    tiles_[cy][cx].terrain = Terrain::Crater;
                }
            }
        }
    }

    disaster_ = {DisasterType::Earthquake, true, 0, tx, ty, radius};
}

void City::update_monster() {
    int target_x = disaster_.radius & 0xFFFF;
    int target_y = (disaster_.radius >> 16) & 0xFFFF;

    int cx = disaster_.x;
    int cy = disaster_.y;

    // Simple step toward target
    if (cx < target_x) cx++;
    else if (cx > target_x) cx--;
    if (cy < target_y) cy++;
    else if (cy > target_y) cy--;

    // Destroy 2-tile wide path at current position
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int px = cx + dx;
            int py = cy + dy;
            if (in_bounds(px, py)) {
                tiles_[py][px].terrain = Terrain::Crater;
                tiles_[py][px].zone = Zone::None;
                tiles_[py][px].density = 0;
                tiles_[py][px].plant_type = PowerPlantType::None;
            }
        }
    }

    disaster_.x = cx;
    disaster_.y = cy;
    disaster_.timer++;

    // Check if reached target
    if (cx == target_x && cy == target_y) {
        disaster_.active = false;
        disaster_.type = DisasterType::None;
    }
}

} // namespace sim
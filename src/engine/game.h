#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "engine/timestep.h"
#include "gfx/cityview.h"
#include "gfx/font.h"
#include "gfx/titlescr.h"
#include "sim/city.h"
#include "ui/rom_font_renderer.h"
#include "ui/budget_panel.h"
#include "ui/population_panel.h"
#include "ui/rci_panel.h"

struct SDL_Window;
struct SDL_Renderer;

class Game {
public:
    enum class Tool { Bulldoze, Road, Residential, Commercial, Industrial };

    enum class GameState { TitleScreen, MainMenu, Playing };

    struct Config {
        int window_w = 800;
        int window_h = 600;
        int fps      = 50;     // SNES runs at ~50fps
        std::string title = "SimCity";
        std::string rom_path;
        bool use_rom_map = true;   // seed terrain from ROM scenario
        bool skip_menu = false;    // skip title/menu for headless testing
        int palette_block = 5;     // BG palette block for city layer (0-13)
        int palette_sub = 0;       // base sub-palette; water uses sub+1
    };

    explicit Game(const Config& cfg);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    bool init();
    int  run();
    void shutdown();

    // Programming interface used by the headless smoke test.
    sim::City& city() { return city_; }
    gfx::CityView* view() const { return view_.get(); }

private:
    void handle_events();
    void update(double dt);
    void render();
    void present();

    void apply_tool(int tx, int ty);
    void step_simulation();
    void draw_hud();

    // State machine
    void handle_events_title();
    void handle_events_menu();
    void handle_events_playing();
    void update_title(double dt);
    void update_menu(double dt);
    void update_playing(double dt);
    void render_title();
    void render_menu();
    void render_playing();
    void build_title_texture();
    void start_new_city();
    void start_practice();

    Config cfg_;
    bool   running_  = false;
    uint64_t frames_ = 0;

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    sim::City     city_;
    gfx::RomAssets rom_assets_;                // extracted tiles + palettes (valid after init)
    std::unique_ptr<gfx::CityView> view_;

    gfx::TitleScreenData title_data_;          // extracted title layers + palette (T021)
    SDL_Texture*         title_tex_ = nullptr; // uploaded title frame

    gfx::FontData        font_data_;           // extracted font glyph mapping (T022)

    GameState state_ = GameState::TitleScreen;
    int       title_timer_ = 0;           // frames on title screen
    int       menu_selection_ = 0;        // 0-4 for main menu options
    static constexpr int kMenuOptionCount = 5;

    Tool    tool_       = Tool::Road;
    bool    paused_     = false;
    bool    dirty_      = true;
    bool    mouse_down_ = false;
    int     last_tile_x_ = -1;
    int     last_tile_y_ = -1;
    int     cam_x_      = 0;
    int     cam_y_      = 0;
    double  sim_accum_  = 0.0;
    uint32_t sim_month_ = 0;
    Tool    last_tool_print_   = Tool::Road;
    bool    last_paused_print_ = false;

    // Disaster visual state
    int     shake_timer_ = 0;
    int     shake_x_ = 0;
    int     shake_y_ = 0;
    sim::DisasterType last_disaster_ = sim::DisasterType::None;
    int     disaster_display_timer_ = 0;

    // UI panels
    ui::RomFontRenderer font_renderer_;
    std::unique_ptr<ui::BudgetPanel> budget_panel_;
    std::unique_ptr<ui::PopulationPanel> population_panel_;
    std::unique_ptr<ui::RciPanel> rci_panel_;
};
#include "engine/game.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <snes/scenariomap.h>
#include <snes/rom.h>

using namespace engine;

namespace {

constexpr double kSimSecondsPerMonth = 1.0;   // one game month per second
constexpr int kCamStep = 24;                   // scroll speed (px/frame)
constexpr int kTitleDurationFrames = 150;     // ~3 seconds at 50fps

const char* tool_name(Game::Tool t) {
    switch (t) {
        case Game::Tool::Bulldoze:     return "bulldoze";
        case Game::Tool::Road:         return "road";
        case Game::Tool::Residential:  return "residential";
        case Game::Tool::Commercial:   return "commercial";
        case Game::Tool::Industrial:   return "industrial";
    }
    return "?";
}

} // namespace

Game::Game(const Config& cfg) : cfg_(cfg) {}
Game::~Game() { shutdown(); }

bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    window_ = SDL_CreateWindow(
        cfg_.title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg_.window_w, cfg_.window_h, SDL_WINDOW_SHOWN);
    if (!window_) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    // Load ROM assets (tiles + palettes) when enabled.
    if (cfg_.use_rom_map && !cfg_.rom_path.empty()) {
        snes::SnesRom rom(cfg_.rom_path);
        if (rom.load()) {
            if (gfx::load_rom_assets(rom, rom_assets_)) {
                rom_assets_.palette_block = cfg_.palette_block;
                rom_assets_.palette_sub = cfg_.palette_sub;
                fprintf(stderr, "Loaded ROM asset tiles+palettes (palette block=%d sub=%d)\n", cfg_.palette_block, cfg_.palette_sub);
            } else {
                fprintf(stderr, "Warning: failed to extract ROM assets, using flat colours\n");
            }
        } else {
            fprintf(stderr, "Warning: failed to load ROM, using generated\n");
        }
    }

    view_ = std::make_unique<gfx::CityView>(
        renderer_, &city_, cfg_.window_w, cfg_.window_h,
        rom_assets_.valid ? &rom_assets_ : nullptr);
    if (!view_->init()) {
        fprintf(stderr, "CityView init failed: %s\n", SDL_GetError());
        return false;
    }

    // Load ROM title screen (T021): real extracted graphics instead of the
    // invented placeholder logo.
    if (cfg_.use_rom_map && !cfg_.rom_path.empty()) {
        snes::SnesRom rom(cfg_.rom_path);
        if (rom.load() && gfx::load_title_screen(rom, title_data_)) {
            build_title_texture();
            fprintf(stderr, "Loaded ROM title screen\n");
        } else {
            fprintf(stderr, "Warning: failed to extract ROM title screen\n");
        }
    }

    // Initialize UI panels
    budget_panel_ = std::make_unique<ui::BudgetPanel>();
    population_panel_ = std::make_unique<ui::PopulationPanel>();
    rci_panel_ = std::make_unique<ui::RciPanel>();

    if (cfg_.skip_menu) {
        state_ = GameState::Playing;
        // Load scenario terrain for headless test
        if (cfg_.use_rom_map && !cfg_.rom_path.empty()) {
            snes::SnesRom rom(cfg_.rom_path);
            if (rom.load()) {
                uint16_t terrain_map[120 * 100];
                if (snes::load_scenario_terrain(rom, 0, terrain_map)) {
                    city_.apply_terrain_map(terrain_map);
                    fprintf(stderr, "Loaded scenario terrain from ROM\n");
                }
            }
        }
    } else {
        state_ = GameState::TitleScreen;
        title_timer_ = 0;
    }

    // Initialize font renderer with ROM font (T022)
    if (rom_assets_.valid) {
        if (gfx::extract_font(rom_assets_, font_data_)) {
            font_renderer_.set_font(&font_data_, &rom_assets_);
            fprintf(stderr, "Loaded ROM font\n");
        } else {
            fprintf(stderr, "Warning: failed to extract ROM font\n");
        }
    }

    running_ = true;
    return true;
}

void Game::handle_events() {
    switch (state_) {
    case GameState::TitleScreen: handle_events_title(); break;
    case GameState::MainMenu:    handle_events_menu();    break;
    case GameState::Playing:     handle_events_playing(); break;
    }
}

void Game::handle_events_title() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            running_ = false;
            break;
        case SDL_KEYDOWN:
        case SDL_MOUSEBUTTONDOWN:
            // Any key or click advances to menu
            state_ = GameState::MainMenu;
            menu_selection_ = 0;
            break;
        }
    }
}

void Game::handle_events_menu() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            running_ = false;
            break;
        case SDL_KEYDOWN:
            switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE:
                running_ = false;
                break;
            case SDLK_UP:
            case SDLK_w:
                menu_selection_ = (menu_selection_ - 1 + kMenuOptionCount) % kMenuOptionCount;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                menu_selection_ = (menu_selection_ + 1) % kMenuOptionCount;
                break;
            case SDLK_RETURN:
            case SDLK_SPACE:
                switch (menu_selection_) {
                case 0: start_new_city(); break;      // New City
                case 1: /* Load City - stub */ break; // Load City (T012)
                case 2: /* Scenario - stub */ break;  // Scenario (T011)
                case 3: start_practice(); break;      // Practice
                case 4: running_ = false; break;      // Quit
                }
                break;
            default: break;
            }
            break;
        }
    }
}

void Game::handle_events_playing() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            running_ = false;
            break;
        case SDL_KEYDOWN:
            switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE:
                state_ = GameState::MainMenu;
                menu_selection_ = 0;
                break;
            case SDLK_1: tool_ = Tool::Bulldoze;    break;
            case SDLK_2: tool_ = Tool::Road;        break;
            case SDLK_3: tool_ = Tool::Residential; break;
            case SDLK_4: tool_ = Tool::Commercial;  break;
            case SDLK_5: tool_ = Tool::Industrial;  break;
            case SDLK_SPACE: paused_ = !paused_; break;
            case SDLK_6: city_.trigger_disaster(sim::DisasterType::Meteor); break;
            case SDLK_7: city_.trigger_disaster(sim::DisasterType::Monster); break;
            // Debug hotkeys (PC port only — NOT part of the original SNES menu):
            // cycle the city-layer palette (F1 = sub-palette, F2 = block) so
            // the palette pairing can be inspected live. See docs/REQUIREMENTS.md.
            case SDLK_F1:
                if (rom_assets_.valid) {
                    cfg_.palette_sub = (cfg_.palette_sub + 1) % 8;
                    rom_assets_.palette_sub = cfg_.palette_sub;
                    dirty_ = true;
                    fprintf(stderr, "Palette: block=%d sub=%d\n", cfg_.palette_block, cfg_.palette_sub);
                }
                break;
            case SDLK_F2:
                if (rom_assets_.valid) {
                    cfg_.palette_block = (cfg_.palette_block + 1) % 14;
                    rom_assets_.palette_block = cfg_.palette_block;
                    dirty_ = true;
                    fprintf(stderr, "Palette: block=%d sub=%d\n", cfg_.palette_block, cfg_.palette_sub);
                }
                break;
            case SDLK_LEFT:
            case SDLK_a: cam_x_ -= kCamStep; break;
            case SDLK_RIGHT:
            case SDLK_d: cam_x_ += kCamStep; break;
            case SDLK_UP:
            case SDLK_w: cam_y_ -= kCamStep; break;
            case SDLK_DOWN:
            case SDLK_s: cam_y_ += kCamStep; break;
            default: break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.button == SDL_BUTTON_LEFT) {
                mouse_down_ = true;
                last_tile_x_ = last_tile_y_ = -1;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (ev.button.button == SDL_BUTTON_LEFT) mouse_down_ = false;
            break;
        case SDL_MOUSEMOTION:
            if (mouse_down_) {
                int tx, ty;
                view_->screen_to_tile(ev.motion.x, ev.motion.y, cam_x_, cam_y_, tx, ty);
                if (tx != last_tile_x_ || ty != last_tile_y_) {
                    apply_tool(tx, ty);
                    last_tile_x_ = tx;
                    last_tile_y_ = ty;
                }
            }
            break;
        }
    }

    // clamp camera to map bounds
    const int max_x = view_->world_w() - cfg_.window_w;
    const int max_y = view_->world_h() - cfg_.window_h;
    if (cam_x_ < 0) cam_x_ = 0;
    if (cam_y_ < 0) cam_y_ = 0;
    if (cam_x_ > max_x) cam_x_ = max_x;
    if (cam_y_ > max_y) cam_y_ = max_y;
}

void Game::apply_tool(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= sim::kMapWidth || ty >= sim::kMapHeight) return;
    switch (tool_) {
    case Tool::Bulldoze:    city_.set_terrain(tx, ty, sim::Terrain::Grass); break;
    case Tool::Road:        city_.set_terrain(tx, ty, sim::Terrain::Road);  break;
    case Tool::Residential: city_.set_zone(tx, ty, sim::Zone::Residential); break;
    case Tool::Commercial:  city_.set_zone(tx, ty, sim::Zone::Commercial);  break;
    case Tool::Industrial:  city_.set_zone(tx, ty, sim::Zone::Industrial);  break;
    }
    dirty_ = true;
}

void Game::step_simulation() {
    city_.step_month();
    ++sim_month_;
    sim::Stats s = city_.stats();
    printf("month %u | pop %d | jobs %d | funds %lld | income %lld | upkeep %lld\n",
           sim_month_, s.population, s.jobs, (long long)s.funds,
           (long long)s.income, (long long)s.upkeep);
    fflush(stdout);
    dirty_ = true;

    // Check for active disaster to trigger visual effects
    auto ds = city_.disaster_state();
    if (ds.active) {
        last_disaster_ = ds.type;
        disaster_display_timer_ = 60;  // show for ~1 second at 50fps
        if (ds.type == sim::DisasterType::Meteor) {
            shake_timer_ = 15;  // ~0.3s shake
        } else if (ds.type == sim::DisasterType::Monster) {
            shake_timer_ = 30;  // longer shake for monster
        }
    }
}

void Game::update(double dt) {
    switch (state_) {
    case GameState::TitleScreen: update_title(dt); break;
    case GameState::MainMenu:    update_menu(dt);    break;
    case GameState::Playing:     update_playing(dt); break;
    }
}

void Game::update_title(double dt) {
    (void)dt;
    title_timer_++;
    if (title_timer_ >= kTitleDurationFrames) {
        state_ = GameState::MainMenu;
        menu_selection_ = 0;
    }
}

void Game::update_menu(double dt) {
    (void)dt;
}

void Game::update_playing(double dt) {
    if (!paused_) {
        sim_accum_ += dt;
        if (sim_accum_ >= kSimSecondsPerMonth) {
            sim_accum_ = 0.0;
            step_simulation();
        }
    }

    if (shake_timer_ > 0) {
        shake_x_ = (shake_timer_ & 1) ? 2 : -2;
        shake_y_ = (shake_timer_ & 2) ? 2 : -2;
        shake_timer_--;
    } else {
        shake_x_ = 0;
        shake_y_ = 0;
    }

    if (disaster_display_timer_ > 0) {
        disaster_display_timer_--;
    }
}

void Game::render() {
    switch (state_) {
    case GameState::TitleScreen: render_title(); break;
    case GameState::MainMenu:    render_menu();    break;
    case GameState::Playing:     render_playing(); break;
    }
}

void Game::render_title() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    // T021: ROM-extracted title frame (Layer3 stars + Layer1 skyline +
    // Layer2 logo), stretched to fill the window like the SNES output.
    if (title_tex_) {
        SDL_Rect dst{0, 0, cfg_.window_w, cfg_.window_h};
        SDL_RenderCopy(renderer_, title_tex_, nullptr, &dst);
    }
}

void Game::build_title_texture() {
    gfx::Image img = gfx::make_image(gfx::TitleScreenData::kW, gfx::TitleScreenData::kH);
    gfx::render_title_screen(title_data_, img);
    title_tex_ = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
        img.width, img.height);
    if (title_tex_) {
        SDL_UpdateTexture(title_tex_, nullptr, img.pixels.data(),
                          img.width * static_cast<int>(sizeof(gfx::Color)));
        SDL_SetTextureBlendMode(title_tex_, SDL_BLENDMODE_NONE);
        // Nearest-neighbor for pixel-perfect scaling (SDL 2.0.12+)
        // Fallback: set render scale quality hint
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    }
}

void Game::render_menu() {
    SDL_SetRenderDrawColor(renderer_, 24, 24, 24, 255);
    SDL_RenderClear(renderer_);

    // Draw title at top using ROM font
    const int title_y = 50;
    const int cx = cfg_.window_w / 2;
    font_renderer_.draw_text(renderer_, cx - 80, title_y, "SIMCITY", 0xFF00B4FF); // blue

    const int start_y = 150;
    const int spacing = 50;
    const int option_w = 300;

    static const char* kOptions[] = {
        "New City",
        "Load City",
        "Scenario",
        "Practice",
        "Quit"
    };

    for (int i = 0; i < kMenuOptionCount; ++i) {
        bool selected = (i == menu_selection_);
        int y = start_y + i * spacing;
        int x = cx - option_w / 2;

        uint32_t color = selected ? 0xFFFFFFFF : 0xFF808080; // white vs gray
        if (selected) {
            // Selected: draw indicator
            font_renderer_.draw_text(renderer_, x - 30, y, ">", 0xFFFFFF00); // yellow
        }

        // Option text
        font_renderer_.draw_text(renderer_, x, y, kOptions[i], color);
    }

    // Hint at bottom
    font_renderer_.draw_text(renderer_, cx - 100, cfg_.window_h - 60, "UP/DOWN  ENTER", 0xFF646464);
}

void Game::render_playing() {
    if (dirty_) {
        view_->rebuild();
        dirty_ = false;
    }

    SDL_SetRenderDrawColor(renderer_, 24, 24, 24, 255);
    SDL_RenderClear(renderer_);
    view_->render(cam_x_, cam_y_, shake_x_, shake_y_);
    draw_hud();
}

void Game::draw_hud() {
    sim::Stats s = city_.stats();

    // top status bar (unchanged)
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_Rect bar{0, 0, cfg_.window_w, 22};
    SDL_RenderFillRect(renderer_, &bar);

    // tool indicator: only print on transition
    if (tool_ != last_tool_print_ || paused_ != last_paused_print_) {
        printf("\r[%s]%s\n", tool_name(tool_), paused_ ? " PAUSED" : "");
        fflush(stdout);
        last_tool_print_ = tool_;
        last_paused_print_ = paused_;
    }

    // Disaster display text (below status bar)
    if (disaster_display_timer_ > 0 && last_disaster_ != sim::DisasterType::None) {
        // Draw as simple colored rect since we don't have font rendering
        SDL_SetRenderDrawColor(renderer_, 255, 50, 50, 255);
        SDL_Rect dr{10, 24, 200, 18};
        SDL_RenderFillRect(renderer_, &dr);
    }

    // Render UI panels (screen-space, camera-independent)
    if (budget_panel_) {
        budget_panel_->render(renderer_, s, &font_renderer_, cfg_.window_w, cfg_.window_h);
    }
    if (population_panel_) {
        population_panel_->render(renderer_, s, &font_renderer_, cfg_.window_w, cfg_.window_h);
    }
    if (rci_panel_) {
        rci_panel_->render(renderer_, s, &font_renderer_, cfg_.window_w, cfg_.window_h);
    }
}

void Game::present() {
    SDL_RenderPresent(renderer_);
}

int Game::run() {
    FixedTimestep timestep(static_cast<double>(cfg_.fps));
    uint64_t prev_time   = SDL_GetPerformanceCounter();
    const uint64_t freq  = SDL_GetPerformanceFrequency();

    while (running_) {
        handle_events();

        uint64_t now = SDL_GetPerformanceCounter();
        double elapsed = static_cast<double>(now - prev_time) / static_cast<double>(freq);
        prev_time = now;

        int ticks = timestep.accumulate(elapsed);
        for (int i = 0; i < ticks; ++i) {
            update(1.0 / cfg_.fps);
        }
        render();
        present();
        ++frames_;
        SDL_Delay(1);  // yield to OS
    }
    return static_cast<int>(frames_ % 2147483647);
}

void Game::start_new_city() {
    // Reset simulation state
    city_ = sim::City();
    sim_month_ = 0;
    sim_accum_ = 0.0;
    paused_ = false;
    cam_x_ = 0;
    cam_y_ = 0;
    tool_ = Tool::Road;
    dirty_ = true;

    // Load scenario 0 terrain
    if (cfg_.use_rom_map && !cfg_.rom_path.empty()) {
        snes::SnesRom rom(cfg_.rom_path);
        if (rom.load()) {
            uint16_t terrain_map[120 * 100];
            if (snes::load_scenario_terrain(rom, 0, terrain_map)) {
                city_.apply_terrain_map(terrain_map);
                fprintf(stderr, "Loaded scenario terrain from ROM\n");
            }
        }
    }

    state_ = GameState::Playing;
}

void Game::start_practice() {
    // Same as new city but with more funds and no disasters
    start_new_city();
    city_.set_funds(50000);  // Extra funds for practice mode
}

void Game::shutdown() {
    view_.reset();
    if (title_tex_) { SDL_DestroyTexture(title_tex_); title_tex_ = nullptr; }
    if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (window_)   { SDL_DestroyWindow(window_);     window_ = nullptr; }
    SDL_Quit();
}
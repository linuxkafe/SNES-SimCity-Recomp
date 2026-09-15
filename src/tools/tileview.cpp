#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL2/SDL.h>

#include "gfx/tile.h"
#include "snes/rom.h"

static constexpr int kTilePx = 8;
static constexpr int kScale  = 4;
static constexpr int kDivX   = 10;   // tiles per row
static constexpr int kDivY   = 18;   // rows
static constexpr int kPad    = 2;
static constexpr int kW = kDivX * (kScale * kTilePx + kPad) + kPad;
static constexpr int kH = kDivY * (kScale * kTilePx + kPad) + kPad + 24;

static SDL_Window*   g_win   = nullptr;
static SDL_Renderer* g_render= nullptr;
static SDL_Texture*  g_tex   = nullptr;

static void die(const char* msg) {
    fprintf(stderr, "error: %s: %s\n", msg, SDL_GetError());
    exit(1);
}

static void upload(const gfx::Image& img) {
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
        0, img.width, img.height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) die("CreateRGBSurfaceWithFormat");

    Uint32* px = static_cast<Uint32*>(surf->pixels);
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const gfx::Color& c = img.at(x, y);
            px[y * img.width + x] =
                (static_cast<Uint32>(c.r) << 24) |
                (static_cast<Uint32>(c.g) << 16) |
                (static_cast<Uint32>(c.b) << 8) |
                (static_cast<Uint32>(c.a));
        }
    }
    SDL_UpdateTexture(g_tex, nullptr, surf->pixels, img.width * 4);
    SDL_FreeSurface(surf);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: tileview <rom.sfc> <byte_offset_hex>\n");
        return 1;
    }
    std::string path = argv[1];
    size_t begin = 0;
    if (sscanf(argv[2], "%zx", &begin) != 1) {
        fprintf(stderr, "error: offset must be hex (e.g. 0x30000 or 30000)\n");
        return 1;
    }

    snes::SnesRom rom(path);
    if (!rom.load()) {
        fprintf(stderr, "error: cannot load rom\n");
        return 1;
    }
    begin &= ~0x1Fu;  // align to 32-byte tile boundary

    const size_t max_off = rom.size() > 32 ? rom.size() - 32 : 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL_Init");
    g_win = SDL_CreateWindow("SimCity tileview", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, kW, kH, 0);
    if (!g_win) die("CreateWindow");
    g_render = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_SOFTWARE);
    if (!g_render) die("CreateRenderer");
    g_tex = SDL_CreateTexture(g_render, SDL_PIXELFORMAT_RGBA32,
                              SDL_TEXTUREACCESS_STREAMING, kW, kH);
    if (!g_tex) die("CreateTexture");

    gfx::Palette16 pal = gfx::default_palette();

    bool running = true;
    bool autoexit = getenv("SIMCITY_TILEVIEW_AUTOEXIT") != nullptr;
    int frame_count = 0;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_RIGHT: case SDLK_SPACE:
                case SDLK_PAGEDOWN:
                    begin = (begin + 8 * 32) % (max_off + 1);
                    break;
                case SDLK_LEFT: case SDLK_PAGEUP:
                    begin = (begin >= 8 * 32) ? begin - 8 * 32 : (max_off / 32) * 32;
                    break;
                case SDLK_DOWN:
                    begin = (begin + 32) % (max_off + 1);
                    break;
                case SDLK_UP:
                    begin = (begin >= 32) ? begin - 32 : (max_off / 32) * 32;
                    break;
                default: break;
                }
            }
        }

        gfx::Image frame = gfx::make_image(kW, kH);

        for (int i = 0; i < kDivX * kDivY; ++i) {
            size_t off = begin + i * 32;
            if (off + 32 > rom.size()) break;
            int tx = kPad + (i % kDivX) * (kScale * kTilePx + kPad);
            int ty = kPad + (i / kDivX) * (kScale * kTilePx + kPad);
            gfx::Image tile_img = gfx::make_image(kTilePx, kTilePx);
            gfx::blit_tile(tile_img, 0, 0, rom.data().data() + off, pal);
            // scale 4x into frame
            for (int sy = 0; sy < kTilePx; ++sy) {
                for (int sx = 0; sx < kTilePx; ++sx) {
                    gfx::Color c = tile_img.at(sx, sy);
                    for (int dy = 0; dy < kScale; ++dy)
                        for (int dx = 0; dx < kScale; ++dx)
                            frame.at(tx + sx * kScale + dx, ty + sy * kScale + dy) = c;
                }
            }
            // offset label
            char lbl[32];
            snprintf(lbl, sizeof lbl, "%04zX", off);
            // small 8x8 font for tile index (skip; keep simple)
            (void)lbl;
        }

        upload(frame);
        SDL_RenderClear(g_render);
        SDL_RenderCopy(g_render, g_tex, nullptr, nullptr);
        SDL_RenderPresent(g_render);
        SDL_Delay(16);
        if (autoexit && ++frame_count >= 3) running = false;
    }

    SDL_DestroyTexture(g_tex);
    SDL_DestroyRenderer(g_render);
    SDL_DestroyWindow(g_win);
    SDL_Quit();
    return 0;
}
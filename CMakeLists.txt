cmake_minimum_required(VERSION 3.15)
project(simcity_port LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

include_directories(src)

find_package(SDL2 REQUIRED)

add_library(snes_rom
    src/snes/rom.cpp
)

add_library(snes_decompress
    src/snes/decompress.cpp
    src/snes/scenariomap.cpp
)
target_include_directories(snes_decompress PUBLIC src)
target_link_libraries(snes_decompress PUBLIC snes_rom)

add_library(simcity_sim
    src/sim/city.cpp
)
target_include_directories(simcity_sim PUBLIC src)

add_library(simcity_gfx
    src/gfx/tile.cpp
    src/gfx/rom_assets.cpp
    src/gfx/cityview.cpp
    src/gfx/titlescr.cpp
    src/gfx/font.cpp
    src/gfx/building_sprites.cpp
)
target_include_directories(simcity_gfx PUBLIC src)
target_link_libraries(simcity_gfx PUBLIC snes_rom snes_decompress simcity_sim SDL2::SDL2)

add_library(simcity_ui
    src/ui/panel.h
    src/ui/rect_font_renderer.cpp
    src/ui/rom_font_renderer.cpp
    src/ui/budget_panel.cpp
    src/ui/population_panel.cpp
    src/ui/rci_panel.cpp
)
target_include_directories(simcity_ui PUBLIC src)
target_link_libraries(simcity_ui PUBLIC simcity_sim simcity_gfx SDL2::SDL2)

add_library(simcity_engine
    src/engine/game.cpp
    src/engine/timestep.cpp
)
target_include_directories(simcity_engine PUBLIC src)
target_link_libraries(simcity_engine PUBLIC simcity_ui simcity_gfx snes_decompress SDL2::SDL2)

add_executable(rominfo
    src/tools/rominfo.cpp
)
target_link_libraries(rominfo snes_rom)

add_executable(tileview
    src/tools/tileview.cpp
)
target_link_libraries(tileview simcity_gfx SDL2::SDL2)

add_executable(rommap
    src/tools/rommap.cpp
)
target_link_libraries(rommap snes_decompress snes_rom)

add_executable(simcity
    src/tools/simcity.cpp
)
target_link_libraries(simcity simcity_engine SDL2::SDL2)

add_executable(test_rom
    tests/test_rom.cpp
)
target_link_libraries(test_rom snes_rom)

add_executable(test_tile
    tests/test_tile.cpp
)
target_link_libraries(test_tile simcity_gfx)

add_executable(test_title
    tests/test_title.cpp
)
target_link_libraries(test_title simcity_gfx snes_rom)

add_executable(test_font
    tests/test_font.cpp
)
target_link_libraries(test_font simcity_gfx simcity_ui simcity_sim snes_rom)

add_executable(test_building_sprites
    tests/test_building_sprites.cpp
)
target_link_libraries(test_building_sprites simcity_gfx simcity_sim snes_rom)

add_executable(test_timestep
    tests/test_timestep.cpp
)
target_link_libraries(test_timestep simcity_engine)

add_executable(test_city
    tests/test_city.cpp
)
target_link_libraries(test_city simcity_sim)

add_executable(test_decompress
    tests/test_decompress.cpp
)
target_link_libraries(test_decompress snes_decompress snes_rom)

add_executable(test_ui
    tests/test_ui.cpp
)
target_link_libraries(test_ui simcity_ui simcity_sim)

enable_testing()
add_test(NAME test_rom COMMAND test_rom)
add_test(NAME test_tile COMMAND test_tile)
add_test(NAME test_title COMMAND test_title)
add_test(NAME test_font COMMAND test_font)
add_test(NAME test_building_sprites COMMAND test_building_sprites)
add_test(NAME test_timestep COMMAND test_timestep)
add_test(NAME test_city COMMAND test_city)
add_test(NAME test_decompress COMMAND test_decompress)
add_test(NAME test_ui COMMAND test_ui)
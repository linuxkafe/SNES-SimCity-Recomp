cmake_minimum_required(VERSION 3.20)
project(SimCitySNESRecomp C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Release pin for lobby matching. Release builds stamp the GAME_VERSION file so two
# peers can prove they are the same build; everything else stamps "dev".
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/GAME_VERSION" _GAME_VER_RAW)
string(STRIP "${_GAME_VER_RAW}" GAME_RELEASE_VERSION)
set(SNESRECOMP_BUILD_VERSION "" CACHE STRING
    "Lobby release pin (empty = Release uses VERSION, else dev)")
if(SNESRECOMP_BUILD_VERSION STREQUAL "")
    get_property(_is_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_is_multi)
        set(SNESRECOMP_BUILD_VERSION "$<IF:$<CONFIG:Release>,${GAME_RELEASE_VERSION},dev>")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(SNESRECOMP_BUILD_VERSION "${GAME_RELEASE_VERSION}")
    else()
        set(SNESRECOMP_BUILD_VERSION "dev")
    endif()
endif()

# snesrecomp framework integration
set(SNESRECOMP_ROOT "${CMAKE_SOURCE_DIR}/snesrecomp" CACHE PATH
    "Path to the snesrecomp checkout used by this game")
if(NOT EXISTS "${SNESRECOMP_ROOT}/runner/runner.cmake")
    message(FATAL_ERROR
        "snesrecomp submodule missing at ${SNESRECOMP_ROOT}.\n"
        "Run: git submodule update --init --recursive")
endif()

# Mod packages are part of every project, not an option. The launcher's Mods
# page, the .snesmod loader, the trusted-plugin registry and netplay's mod-set
# gate all hang off this one switch; a build without it cannot exchange mods
# with a peer that has them, and the framework says so at every lobby init.
# Must precede the include: runner.cmake reads it to add the loader sources.
set(SNESRECOMP_ENABLE_MODS ON CACHE BOOL
    "Mod package loader + trusted plugins (required for every title)" FORCE)
include(${SNESRECOMP_ROOT}/runner/runner.cmake)

# Main recompiled game executable
add_executable(SimCitySNESRecomp
    ${SNESRECOMP_RUNNER_SOURCES}
    src/main.c
    src/host_contract.c
    src/game_rtl.c
    src/gen_stubs.c
)

# ROM identity. rom_identity.txt is the one place the digests live: the build
# turns it into snesrecomp_rom_identity.h (SNESRECOMP_ROM_EXPECTED_SHA256 and
# friends) for src/main.c, and tools/regen.sh, scripts/package_release.sh and
# the release workflow read the same file. A revision bump is one edit there.
snesrecomp_rom_identity(SimCitySNESRecomp ${CMAKE_SOURCE_DIR}/rom_identity.txt)

# Generated recompiler output. Not committed: it is derived from a copyrighted
# ROM, so every developer regenerates it locally from their own copy -- and a
# release does not ship it either. With -DSNESRECOMP_SETUP_HOST=ON and an empty
# src/gen/, this builds the SETUP HOST instead: the same executable minus the
# recompiled code, whose only path is the launcher's Generate & rebuild wizard.
# That is what release zips contain; the player's machine produces the
# playable binary from their own ROM. Without the option an empty src/gen/ is
# still a hard error (the framework owns the message).
snesrecomp_target_generated_code(SimCitySNESRecomp ${CMAKE_SOURCE_DIR}/src/gen)

# The mod catalog. This names WHERE the packages are; the framework owns the
# layout they are staged into beside the executable, and stages them on every
# build OF THE EXECUTABLE -- including the launcher's own Generate & rebuild on
# a player's machine, which builds only this target and would skip a sibling
# ALL target. mods/preloaded/packages/ holds reviewed, default-disabled
# packages; players install .snesmod archives through the launcher.
# snesrecomp/docs/MOD_PACKAGES.md.
snesrecomp_target_mod_catalog(SimCitySNESRecomp ${CMAKE_SOURCE_DIR}/mods/preloaded)

target_include_directories(SimCitySNESRecomp PRIVATE
    ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
    # common_rtl.c includes "config.h": the desktop key-map / Config contract
    # the framework host supplies (snesrecomp_target_desktop_host below).
    ${SNESRECOMP_ROOT}/runner/src/desktop
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/recomp
)
target_link_libraries(SimCitySNESRecomp PRIVATE ${SNESRECOMP_RUNNER_LIBRARIES})
target_compile_definitions(SimCitySNESRecomp PRIVATE
    SNES_GAME_VERSION="${SNESRECOMP_BUILD_VERSION}"
    SNES_GAME_TITLE="SimCity SNES Recomp"
)
snesrecomp_target_sdl(SimCitySNESRecomp)

# The desktop host: launcher flow, ROM resolution, config.ini + [KeyMap],
# window + SDL/OpenGL presenters, audio, gamepads, the save-state browser and
# rewind filmstrip, the pacing clock, crash handlers and the post-mortem
# report. src/main.c is the shim that names this title and supplies its
# hooks; the host is the framework's and improves on a submodule pull.
snesrecomp_target_desktop_host(SimCitySNESRecomp)

# rom.cfg (the ROM the wizard set this project up from; git-ignored) is
# staged beside the executable when none is there yet, so the first launch
# opens on that dump rather than asking for one.
snesrecomp_target_rom_cache(SimCitySNESRecomp)

# ============================================================
# Testing
# ============================================================
enable_testing()

# Deterministic Replay Test: verifies the recompiled SimCity produces
# identical state traces across runs with the same inputs.
add_executable(test_deterministic_replay
    tests/test_deterministic_replay.c
)
add_test(NAME test_deterministic_replay COMMAND test_deterministic_replay)

# Custom target to run regeneration
add_custom_target(regen
    COMMAND bash ${CMAKE_SOURCE_DIR}/tools/regen.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Regenerate src/gen/ from ROM via snesrecomp"
    VERBATIM
)
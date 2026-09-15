---
ticket: T029
title: Modding API — Lua Bindings for sim::City
sprint: sprint-03
priority: high
status: backlog
created: 2026-09-14
source: T027 decision — modding requirement addressed via owned C++ API
---

# T029 — Modding API: Lua Bindings for sim::City

## Context
Modders need to read/write simulation state, hook events, and override rules. Reimplementation C++ core is ideal for this — clean APIs, deterministic, testable. Lua provides sandboxed scripting with reload support.

## Acceptance Criteria
- [ ] Expose `sim::City` read API: `city:get_tile(x,y)`, `city:stats()`, `city:funds()`
- [ ] Expose write API: `city:set_zone(x,y,type)`, `city:set_tax_rate(rate)`, `city:trigger_disaster(type)`
- [ ] Event hooks: `on_month(city)`, `on_zone_change(x,y,old,new)`, `on_disaster(type,x,y)`
- [ ] Rule overrides via config: tax curves, growth formulas, disaster chances
- [ ] Sandbox: no file I/O, no native calls, deterministic execution
- [ ] Hot-reload: `mod_reload` command without restart
- [ ] Example mods: "Unlimited Money", "Hardcore Disasters", "Custom Growth"
- [ ] 6/6 ctest + headless smoke pass

## Technical Approach
- Add `sol2` (header-only Lua C++ binding) or `lua.hpp` + manual bindings
- `sim::ModManager` owns `lua_State`, loads `mods/*.lua`
- `sim::City` gets `ModManager*` pointer, calls hooks at deterministic points
- API versioning: `mod_api_version = 1` in mod manifest
- Determinism: Lua RNG seeded from `city.month_`, no `os.time()`
- Headless test: run mod that asserts state after N months

## Dependencies
- sol2 (header-only, MIT) or Lua 5.4 (MIT)
- T005 (sim core) — stable API
- No recompilation dependency

## Estimation
- Complexity: medium (2-8h)
- Risk: low
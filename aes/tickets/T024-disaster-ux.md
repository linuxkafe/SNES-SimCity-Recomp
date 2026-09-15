---
ticket: T024
title: Disaster UX — Earthquake or gate random disasters
sprint: sprint-02
priority: medium
status: in-progress
created: 2026-09-14
source: T020 finding F5
---

# T024 — Disaster UX

## Context
T020 audit confirmed F5: random meteor/monster disasters (2%→15% monthly chance) cause camera shake and red rectangle flash, indistinguishable from "random earthquakes" to the user. The original SNES SimCity has an earthquake disaster type. This ticket either implements earthquake or gates random disasters.

## Research Summary
- `city.cpp:477-494`: `process_disasters()` rolls monthly with threshold = 200 + pop/100*50, capped at 1500 (15%). At pop=1000 → 7% chance. Too frequent.
- `game.cpp:456-461`: Visual is a red rect (255,50,50) - no text/label.
- Original SNES: Has earthquake disaster (ground shake, cracks roads/buildings). Meteor/monster exist but rarer.

## Plan
1. **Reduce disaster frequency**: Cap at 5% (500/10000), remove population scaling or make it gentler.
2. **Add Earthquake disaster type**: Ground shake + terrain cracks (road/building damage).
3. **Update visual notification**: Use ROM font (T022) to render "METEOR STRIKE" / "MONSTER ATTACK" / "EARTHQUAKE" instead of red rect.
4. **Add disaster config option**: Allow disabling automatic disasters.

## Acceptance Criteria
- [ ] Disaster frequency capped at ~5% (configurable)
- [ ] Earthquake disaster implemented (ground shake + road/building cracks)
- [ ] Visual notification uses ROM font with disaster name
- [ ] Config option to disable automatic disasters
- [ ] 9/9 ctest + headless smoke pass

## Implementation
### Files to modify
- `src/sim/city.h` / `.cpp`: Add Earthquake to DisasterType, implement trigger_earthquake()
- `src/engine/game.cpp`: Update disaster display to use font_renderer_ with labels
- `src/tools/simcity.cpp`: Add `--no-disasters` flag

### DisasterType enum update
```cpp
enum class DisasterType { None, Meteor, Monster, Earthquake };
```

### Earthquake effect
- Camera shake (longer duration)
- Crack random roads (Road → Crater) in radius
- Damage buildings (reduce density) in radius

## Known Risks
- Earthquake may need specific terrain damage patterns
- Disabling random disasters changes game balance
- Visual notification needs ROM font (T022) — already done
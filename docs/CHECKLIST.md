# Quality Gate Checklist — SimCity SNES PC Port

## Pre-Commit (Quick)

| Gate | Command | Type | Status |
|------|---------|------|--------|
| Unit Tests | `ctest --output-on-failure` | BLOCKER | required |
| Lint (C++) | `clang-tidy src/ -checks=bugprone-*,performance-*,readability-*,modernize-*,clang-analyzer-* 2>/dev/null || echo "clang-tidy not available"` | BLOCKER | required |
| Format | `clang-format --dry-run --Werror src/ tests/ 2>/dev/null || echo "clang-format not available"` | BLOCKER | required |
| No Debug Code | `grep -r "std::cout\\|printf\\|DEBUG" src/ || true` | BLOCKER | required |
| No TODO in src | `grep -r "TODO\\|FIXME" src/ || true` | BLOCKER | required |

## Pre-Release (Thorough)

| Gate | Command | Type | Status |
|------|---------|------|--------|
| Full Test Suite | `ctest --output-on-failure` | BLOCKER | required |
| Coverage | `ctest -T Test` (requires gcov/lcov) | WARNING | optional |
| Headless Smoke | `SDL_VIDEODRIVER=dummy timeout 3 ./build/simcity "SimCity (USA).sfc"` | BLOCKER | required |
| ROM Validation | `./build/rominfo "SimCity (USA).sfc" | grep -q "Checksum valid YES"` | BLOCKER | required |
| Tile View Smoke | `SIMCITY_TILEVIEW_AUTOEXIT=1 SDL_VIDEODRIVER=dummy ./build/tileview "SimCity (USA).sfc" 0x10000` | BLOCKER | required |
| Docs Updated | `git diff --name-only HEAD~5..HEAD | grep -E "(docs/|CLAUDE\\.md)" || echo "no doc changes"` | WARNING | optional |
| Git Clean | `git status --porcelain | grep -v "^??" | wc -l` | BLOCKER | required |

## Domain-Specific Gates

### AES Compliance
| Gate | Criteria | Type | Status |
|------|----------|------|--------|
| Ticket Artifacts | TXXX-name.md, plan, build, verify, review, learn exist for every non-trivial ticket | BLOCKER | required |
| Kanban Updated | Ticket statuses match actual state in aes/kanban.md | BLOCKER | required |
| Sprint Updated | Sprint file reflects ticket completions | BLOCKER | required |
| Shadow Docs | SD-META-*/SD-CI-* created for key insights | WARNING | optional |
| Index Fresh | `make index` runs without errors | WARNING | optional |

### C++ Specific
| Gate | Criteria | Type | Status |
|------|----------|------|--------|
| C++17 Compliance | No C++20 features used | BLOCKER | required |
| No CRT Warnings | Clean build with `-Wall -Wextra -Wpedantic` | BLOCKER | required |
| No Memory Leaks | Valgrind clean on headless run (if available) | WARNING | optional |

### Legal
| Gate | Criteria | Type | Status |
|------|----------|------|--------|
| No ROM in Repo | `git ls-files | grep -E "\.(sfc|smc|bin)$" || true` | BLOCKER | required |
| No Ripped Assets | `git ls-files | grep -E "assets/|data/|tiles/" || true` | BLOCKER | required |

## Configuration

```make
# In docs/CHECKLIST.md — configure per-project
BLOCKER_GATES = unit_tests lint format no_debug no_todo headless_smoke rom_validation tile_smoke git_clean aes_ticket kanban sprint
WARNING_GATES = coverage docs aes_shadow index cpp17 crt_warnings mem_leaks
```
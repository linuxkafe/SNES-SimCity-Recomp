.PHONY: build debug test test-rom perf clock clock-self-test check-claims \
        check-claims-self-test check-causes check-causes-self-test \
        retraction-count review-check clean doctor

BUILD_DIR := build

# Release by default. This used to hardcode -DCMAKE_BUILD_TYPE=Debug, so everyone
# who followed the README got a -g build with no optimisation: measured 42.4 fps
# against Release's 55.8 on the same machine, same ROM, same 600 frames, and
# 60.1 fps for the same Release binary on the Steam Deck. The two builds were
# proven byte-identical on the guest first (128 KB WRAM image and every presented
# crc32, across attract/menu/naming at 3000 frames), so the default is the fast
# one rather than the safe-looking one. Override with BUILD_TYPE=Debug.
BUILD_TYPE ?= Release

# A clean build dir does not configure without this on any machine lacking
# libxtst-dev: SDL3's build stops with "Couldn't find dependency package for
# XTEST". The checked-in build/ only worked because the flag was already cached
# in it, which is how `make clean && make build` came to be broken from scratch.
# XTEST is SDL's synthetic-input path and nothing in this project uses it, so
# turning it off states a real precondition instead of hiding a missing package.
SDL_X11_XTEST ?= OFF

build:
	@mkdir -p $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DSDL_X11_XTEST=$(SDL_X11_XTEST)
	cmake --build $(BUILD_DIR) -j$$(nproc)

# The unoptimised build. Kept because -O0 and -O3 disagreeing is exactly the
# signal you want when hunting undefined behaviour, not something to discover
# by accident in a shipping binary.
debug:
	@$(MAKE) --no-print-directory build BUILD_TYPE=Debug

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

# Needs the ROM, so it cannot be a ctest: the ROM is never committed, and a
# test needing it would break for every developer without one. This is the gate
# that would have caught T057, where src/gen/ was regenerated to 216 AOT
# functions, the screen went black, and ctest stayed green.
test-rom: build
	scripts/verify-rom-render.sh

# Performance gate. Not a ctest, for the same reason test-rom is not: it needs
# the ROM, and its threshold is a property of the machine it runs on. Override
# with PERF_MIN_FPS when the host is slower or faster than the reference.
perf: build
	scripts/perf-gate.sh

# The gate that can see the game being dead. Not a ctest for the same two
# reasons as the others: it needs the ROM, and it needs the game to actually be
# reached - which takes 3,600 frames of scripted input. Every existing gate
# inspects frames 30-800 and so passes while the city sits frozen.
#
# Run --self-test after changing anything in the script: there is no build in
# this repository where the clock advances, so the only way to know the
# detector still sees a live screen is to make it prove that on a window that
# is alive.
clock: build
	scripts/clock-gate.sh

clock-self-test: build
	scripts/clock-gate.sh --self-test --frames 1200

# Evidence-integrity gate. Cheap, needs no ROM and no build, and it is the guard
# whose absence let eleven retractions stand: it fails when a script or doc
# asserts a claim that scripts/retracted-claims.tsv records as refuted, without
# a retraction marker where the claim is made. A gate that teaches a retracted
# cause is worse than one that reports none, because the wrong cause is what
# the next session starts from.
#
# Run it with --self-test after editing either the checker or the ledger: a
# guard that has never been seen to fail has not been tested.
check-claims: build
	scripts/check-retracted-claims.sh

# The second evidence gate, and the one check-retracted-claims.sh cannot be.
#
# check-retracted-claims.sh is LEXICAL: it holds strings that were already
# refuted. A cause claim nobody has retracted yet is invisible to it. This one
# asks the structural question instead - does this line assert a CAUSE, and does
# it say where the cause came from - so it needs no ledger row to catch a new
# false claim.
#
# Run it with --self-test. It seeds from git history (the tree at 9624f0e) and
# asserts both directions: that it fires there, and that it is clean here. A
# guard that has never been seen to fail has not been tested.
check-causes:
	scripts/check-cause-claims.sh

check-causes-self-test:
	scripts/check-cause-claims.sh --self-test

# The retraction count, computed. Not prose, not a git-log grep, not a tally:
# the number of ledger rows whose status is `refuted`. Added 2026-10-02 after
# this project was found carrying four prose counts of its own retractions,
# none in agreement. See scripts/retracted-claims.tsv for the definition.
retraction-count:
	scripts/check-retracted-claims.sh --count

check-claims-self-test:
	scripts/check-retracted-claims.sh --self-test --open-labels

# Validates the findings in docs/review/REVIEW-*.md against the tree. Intended
# to be run by someone other than the author of the review.
review-check:
	docs/review/validate-findings.sh

clean:
	rm -rf $(BUILD_DIR)

doctor:
	@echo "== Toolchain =="
	@which g++ cmake make python3 2>/dev/null || true
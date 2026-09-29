.PHONY: build test test-rom clean doctor

BUILD_DIR := build

build:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . -j$$(nproc)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

# Needs the ROM, so it cannot be a ctest: the ROM is never committed, and a
# test needing it would break for every developer without one. This is the gate
# that would have caught T057, where src/gen/ was regenerated to 216 AOT
# functions, the screen went black, and ctest stayed green.
test-rom: build
	scripts/verify-rom-render.sh

clean:
	rm -rf $(BUILD_DIR)

doctor:
	@echo "== Toolchain =="
	@which g++ cmake make python3 2>/dev/null || true
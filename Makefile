.PHONY: build test rominfo clean doctor

BUILD_DIR := build

build:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . -j$$(nproc)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

rominfo: build
	./$(BUILD_DIR)/rominfo "SimCity (USA).sfc"

clean:
	rm -rf $(BUILD_DIR)

doctor:
	@echo "== Toolchain =="
	@which g++ cmake make python3 2>/dev/null || true
	@echo "== SDL2 =="
	@pkg-config --modversion sdl2 2>/dev/null || echo "SDL2 NOT FOUND"
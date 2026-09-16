.PHONY: build test clean doctor

BUILD_DIR := build

build:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . -j$$(nproc)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

doctor:
	@echo "== Toolchain =="
	@which g++ cmake make python3 2>/dev/null || true
BUILD_DIR := build
BUILD_TYPE ?= Release

export http_proxy ?= http://127.0.0.1:7897
export https_proxy ?= $(http_proxy)
export ALL_PROXY ?= $(http_proxy)

.PHONY: all build run format clean

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) --target editor

run: build
	./$(BUILD_DIR)/editor $(ARGS)

format:
	clang-format -i src/*.cpp src/*.hpp

clean:
	rm -rf $(BUILD_DIR)

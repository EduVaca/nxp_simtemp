# Get the absolute path of this file.
WORKSPACE := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
export WORKSPACE

.PHONY: all build clean

# Define the build directory
BUILD_DIR := $(WORKSPACE)/build
FILE_DTBO := $(WORKSPACE)/kernel/dts/nxp-simtemp.dtbo

all: build

build: $(BUILD_DIR)
	@echo "--- Starting build process from $(WORKSPACE) ---"
	# Build artifacts
	./scripts/build.sh
	@echo "--- Build complete ---"

clean:
	@echo "--- Cleaning up build artifacts ---"
	@if [ -d "$(BUILD_DIR)" ]; then \
		echo "Removing $(BUILD_DIR)..."; \
		rm -rfv "$(BUILD_DIR)"; \
	fi

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
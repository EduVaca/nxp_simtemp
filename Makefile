# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

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
	@$(MAKE) -C kernel
	@echo "--- Build complete ---"

install:
	@echo "--- Installing from $(WORKSPACE) ---"
	@$(MAKE) -C kernel modules_install
	@echo "Updating kernel modules' dependencies ..."
	@depmod
	@echo "--- Install complete ---"

clean:
	@echo "--- Cleaning up build artifacts ---"
	@$(MAKE) -C kernel clean
	@if [ -d "$(BUILD_DIR)" ]; then \
		echo "Removing $(BUILD_DIR)..."; \
		rm -rfv "$(BUILD_DIR)"; \
	fi
	@echo "--- Cleaning up build artifacts complete ---"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
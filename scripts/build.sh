#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>



# Get paths to folders
SCRIPT_DIR=$(cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
SRC_PATH=$(dirname "${SCRIPT_DIR}")
BUILD_PATH="${SRC_PATH}/build"

# Get the current kernel version
KERNEL_VERSION=$(uname -r)

# Define a function for colored output
function print_status {
    local status="$1"
    local message="$2"
    case "$status" in
        "ok")
            echo -e "\e[32m[OK]\e[0m $message"
            ;;
        "error")
            echo -e "\e[31m[ERROR]\e[0m $message"
            ;;
        "info")
            echo -e "\e[34m[INFO]\e[0m $message"
            ;;
    esac
}

# Exit point
function local_exit {
    if [ -v BUILD_PATH ]; then
        unset BUILD_PATH
    fi
    if [ -v KERNEL_HEADERS ]; then
        unset KERNEL_HEADERS
    fi
    exit "$1"
}

print_status "info" "--- Build start ---"

# --- Check prerequisites ---
print_status "info" "Checking prerequisites..."

# --- Check Linux Headers ---
print_status "info" "Kernel headers"
if [ -z "${KERNEL_HEADERS}" ]; then
    export KERNEL_HEADERS="/lib/modules/${KERNEL_VERSION}/build"
fi

print_status "info" "Using kernel headers from: ${KERNEL_HEADERS}"

if [ ! -e "${KERNEL_HEADERS}" ]; then
    print_status "error" "Kernel headers are not available"
    local_exit 11
fi

print_status "ok" "Kernel headers"

print_status "info" "Checking build directory..."
if [ -e "${BUILD_PATH}" ]; then
    print_status "info" "Removing build directory ${BUILD_PATH}"
    rm -rf "${BUILD_PATH}"
fi

export BUILD_PATH
print_status "info" "Cleanig ${SRC_PATH}/kernel"
make -C "${SRC_PATH}/kernel" clean

# --- It's just clean-up command ---
case "$1" in
  "-c"|"-C"|"--clean")
    print_status "ok" "Clean complete"
    local_exit 0
    ;;
esac

print_status "info" "Creating build directory ${BUILD_PATH}"
mkdir "${BUILD_PATH}"
if [ ! -e "${BUILD_PATH}" ]; then
    print_status "error" "mkdir ${BUILD_PATH}"
    local_exit 12
fi

print_status "ok" "Build directory"

# --- Compile the Device Tree Overlay ---
print_status "info" "Compiling nxp-simtemp.dtsi ..."

dtc -O dtb -o "${BUILD_PATH}/nxp-simtemp.dtbo" -@ \
    "${SRC_PATH}/kernel/dts/nxp-simtemp.dtsi"
exit_code="$?"

if [ "${exit_code}" -ne 0 ] || [ ! -e "${BUILD_PATH}/nxp-simtemp.dtbo" ]; then
    print_status "error" "Device Tree Overlay compilation"
    local_exit 21
fi

print_status "ok" "nxp-simtemp.dtbo created"

# --- Compile Kernel Mode Driver and testing application ---
print_status "info" "Compiling nxp_simtemp driver ..."
make -C "${SRC_PATH}/kernel"

if [ ! -e "${SRC_PATH}/kernel/nxp_simtemp.ko" ]; then
    print_status "error" "Compiling nxp_simtemp driver ..."
    local_exit 31
fi

print_status "ok" "nxp_simtemp driver"

# --- Check if module signing is needed ---
print_status "info" "Compiling nxp_simtemp driver ..."

KERNEL_CONFIG="/boot/config-${KERNEL_VERSION}"
# Check if the kernel config file exists
if [ -e "${KERNEL_CONFIG}" ]; then
    # Search for CONFIG_MODULE_SIG in the config file
    if grep -q "CONFIG_MODULE_SIG=y" "$KERNEL_CONFIG"; then
        print_status "info" \
            "The current kernel was built with CONFIG_MODULE_SIG=y."
        print_status "info" "Sign the module before loaing"
    else
        print_status "ok" "Kernel can be loaded without signing"
    fi
else
    print_status "info" "Kernel config file not found: $KERNEL_CONFIG"
    print_status "info" \
        "Validate if module signing is needed for your distrubution"
fi

print_status "ok" "--- Build complete ---"
local_exit 0
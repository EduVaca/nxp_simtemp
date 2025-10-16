#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

set -e  # Exit immediately if a command exits with a non-zero status.

# Get paths to folders
SCRIPT_DIR=$(cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
SRC_PATH=$(dirname "${SCRIPT_DIR}")
BUILD_PATH="${SRC_PATH}/build"

MODULE_FILE="${SRC_PATH}/kernel/nxp_simtemp.ko"
TEST_TOOL="${BUILD_PATH}/nxp_simtemp_test"
MODULE_NAME="nxp_simtemp"
SYSFS_DIR="/sys/devices/platform/simtemp"

# Define a function for colored output
function print_status {
    local status="$1"
    local message="$2"
    case "${status}" in
        "ok")
            echo -e "\e[32m[OK]\e[0m ${message}"
            ;;
        "error")
            echo -e "\e[31m[ERROR]\e[0m ${message}"
            exit 1
            ;;
        "info")
            echo -e "\e[34m[INFO]\e[0m ${message}"
            ;;
    esac
}

# Define a helper function for permission checks
function check_permissions {
    local path="$1"
    local expected_permissions="$2"
    if [ ! -e "${path}" ]; then
        print_status "error" "Sysfs path not found: ${path}"
    fi
    FILE_PERM=$(stat -c "%a" "${path}")
    case "${expected_permissions}" in
        "rw")
            if [ "${FILE_PERM}" -eq 644 ]; then
                print_status "ok" "Path ${path} has read/write permissions."
            else
                print_status "error" \
                    "Path ${path} does not have read/write permissions."
            fi
            ;;
        "ro")
            if [ "${FILE_PERM}" -eq 444 ]; then
                print_status "ok" "Path ${path} has read-only permissions."
            else
                print_status "error" \
                    "Path ${path} does not have read-only permissions."
            fi
            ;;
    esac
}

# --- Check prerequisites ---
print_status "info" "Checking prerequisites..."
if [ "$(id -u)" -ne 0 ]; then
    print_status "error" "This script must be run as root."
fi

if [ ! -f "${MODULE_FILE}" ]; then
    print_status "error" "Kernel module not found: ${MODULE_FILE}"
fi

if [ ! -x "${TEST_TOOL}" ]; then
    print_status "error" "Test tool not found or not executable: ${TEST_TOOL}"
fi

# --- Main test sequence ---
print_status "info" "--- Starting test sequence for ${MODULE_NAME} ---"

# --- Load the module ---
print_status "info" "Loading module ${MODULE_FILE}..."
insmod "${MODULE_FILE}"
print_status "ok" "Module inserted."

# --- Validate module load with lsmod ---
print_status "info" "Validating module load with lsmod..."
if lsmod | grep -q "^${MODULE_NAME} "; then
    print_status "ok" "Module ${MODULE_NAME} is listed by lsmod."
else
    print_status "error" "Module ${MODULE_NAME} not found by lsmod."
fi

# --- Validate sysfs paths and permissions ---
print_status "info" "Validating sysfs paths and permissions..."

check_permissions "${SYSFS_DIR}/sampling_ms" "rw"
check_permissions "${SYSFS_DIR}/threshold_mC" "rw"
check_permissions "${SYSFS_DIR}/mode" "rw"
check_permissions "${SYSFS_DIR}/stats" "ro"

# --- Run test tool commands ---
print_status "info" "Running test tool commands and validating sysfs values..."

# Test 1: Set sampling_ms to 1000
"${TEST_TOOL}" -s 1000
if [ "$(cat "${SYSFS_DIR}/sampling_ms")" -eq 1000 ]; then
    print_status "ok" "sampling_ms was successfully set to 1000."
else
    print_status "info" "Current value: $(cat "${SYSFS_DIR}/sampling_ms")"
    print_status "error" "Failed to set sampling_ms to 1000."
fi

# Test 2: Set threshold_mC to 30000
"${TEST_TOOL}" -t 30000
if [ "$(cat "${SYSFS_DIR}/threshold_mC")" -eq 30000 ]; then
    print_status "ok" "threshold_mC was successfully set to 30000."
else
    print_status "info" "Current value: $(cat "${SYSFS_DIR}/threshold_mC")"
    print_status "error" "Failed to set threshold_mC to 30000."
fi

# Test 3: Set mode to ramp
"${TEST_TOOL}" -m ramp
if [ "$(cat "${SYSFS_DIR}/mode")" == "ramp" ]; then
    print_status "ok" "mode was successfully set to ramp."
else
    print_status "info" "Current value: $(cat "${SYSFS_DIR}/mode")"
    print_status "error" "Failed to set mode to ramp."
fi

# --- Remove the module ---
print_status "info" "Removing module ${MODULE_NAME} with rmmod..."
rmmod "${MODULE_NAME}"
print_status "ok" "Module removed."

# --- Validate module removal with lsmod ---
print_status "info" "Validating module removal with lsmod..."
if ! lsmod | grep -q "^${MODULE_NAME} "; then
    print_status "ok" "Module ${MODULE_NAME} is no longer listed by lsmod."
else
    print_status "error" "Module ${MODULE_NAME} is still listed by lsmod."
fi

# --- Validate sysfs paths are gone ---
print_status "info" "Validating sysfs paths were removed..."
if [ ! -d "${SYSFS_DIR}" ]; then
    print_status "ok" "Sysfs directory ${SYSFS_DIR} was removed."
else
    print_status "error" "Sysfs directory ${SYSFS_DIR} still exists."
fi

print_status "info" "--- All tests passed successfully ---"
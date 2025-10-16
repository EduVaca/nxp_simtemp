#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

# Defines
MODULE_NAME="simtemp"
SYSFS_PATH="/sys/devices/platform/${MODULE_NAME}"

CHAR_DEV="/dev/${MODULE_NAME}"
VAR_MODE="${SYSFS_PATH}/mode"
VAR_SAMPLING_MS="${SYSFS_PATH}/sampling_ms"
VAR_THRESHOLD_MC="${SYSFS_PATH}/threshold_mC"

DEV_PATHS=(\
    "${CHAR_DEV}"\
    "${VAR_MODE}"\
    "${VAR_SAMPLING_MS}"\
    "${VAR_THRESHOLD_MC}"\
)
FILE_MOD="646"

# Define a function for colored output
function print_status {
    local status="${1}"
    local message="${2}"
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

# --- Validate sysfs paths and permissions ---
for DEV_PATH in "${DEV_PATHS[@]}"; do
    print_status "info" "Changing permissions of ${DEV_PATH} to ${FILE_MOD} ..."
    chmod "${FILE_MOD}" "${DEV_PATH}"
    return_code="${?}"
    if [ "${return_code}" -eq 0 ]; then
        status="ok"
    else
        status="error"
    fi
    print_status "${status}" "${DEV_PATH}"
done

exit 0

#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

# ==============================================================================
# LINTING SCRIPT
# This script runs various linters on the project's source code to ensure code
# quality and style consistency. It checks C, DTS, Python, and shell files.
# ==============================================================================

set -e # Exit immediately if a command exits with a non-zero status

# Get paths to folders
SCRIPT_DIR=$(cd -- "$(dirname -- "$(readlink -f -- "$0")")" && pwd)
SRC_PATH=$(dirname ${SCRIPT_DIR})

# ==============================================================================
# Helper functions
# ==============================================================================

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
            ;;
        "info")
            echo -e "\e[34m[INFO]\e[0m ${message}"
            ;;
    esac
}

# Function to check for required command-line tools
check_dependency() {
    if ! command -v "$1" &> /dev/null; then
        echo "Error: Required tool '$1' is not installed."
        echo "Please install '$1' to continue."
        exit 1
    fi
}

# Function to run a linter and check its exit code
run_linter() {
    local linter_name="$1"
    local linter_command="$2"
    local linter_target="$3"

    print_status "info" \
        "================================================================="
    print_status "info" \
        "Running $linter_name on $linter_target..."
    print_status "info" \
        "-----------------------------------------------------------------"

    # Temporarily disable 'set -e' to allow linter to fail without exiting
    set +e
    $linter_command
    local exit_code=$?
    # Re-enable 'set -e'
    set -e

    if [ $exit_code -eq 0 ]; then
        print_status "ok" \
            "$linter_name check passed for $linter_target."
    else
        print_status "error" \
            "$linter_name check failed for $linter_target with errors."
    fi
    echo
}

# ==============================================================================
# Dependency checks
# ==============================================================================

print_status "info" "Checking for required linting tools..."
check_dependency "cpplint"
check_dependency "dtc"
check_dependency "pylint"
check_dependency "shellcheck"
print_status "info" "All dependencies are present."
echo

# ==============================================================================
# Linting tasks
# ==============================================================================

# Lint C code in the 'kernel' directory
run_linter "C++ Linter" "cpplint --recursive ${SRC_PATH}/kernel" "C files"

# Lint Device Tree Source (DTS) files in the 'kernel' directory
# The `dtc` command can be used to validate DTS file syntax by compiling it
# and checking for errors.
DTS_FILES=$(find ${SRC_PATH}/kernel -name "*.dtsi")
for dts_file in $DTS_FILES; do
    run_linter "DTS Compiler (as linter)" \
        "dtc -I dts -O dtb -o /dev/null -q $dts_file" "$dts_file"
done

# Lint Python code in the 'user' directory
run_linter "PyLint" "pylint ${SRC_PATH}/user" "Python files"

# Lint shell scripts in the 'scripts' directory
# The script finds all files ending in .sh within the scripts folder,
# including the script's own location.
SHELL_SCRIPTS=$(find ${SRC_PATH}/scripts -name "*.sh")
for script in $SHELL_SCRIPTS; do
    run_linter "ShellCheck" "shellcheck $script" "$script"
done

# ==============================================================================
# Completion
# ==============================================================================

print_status "info" \
    "================================================================="
print_status "info" \
    "All linting tasks completed successfully!"
print_status "info" \
    "================================================================="

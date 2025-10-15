#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

echo "--- Building simtemp ---"

# 1. Compile the Device Tree Overlay
echo "Compiling nxp-simtemp.dtsi..."
dtc -O dtb -o ${WORKSPACE}/build/nxp-simtemp.dtbo -@ ${WORKSPACE}/kernel/dts/nxp-simtemp.dtsi
if [ $? -ne 0 ]; then
    echo "Error: Device Tree Overlay compilation failed."
    exit 1
fi
echo "nxp-simtemp.dtbo created."

echo "--- Build complete ---"

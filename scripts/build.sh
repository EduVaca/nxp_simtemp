#!/bin/bash

echo "--- Building simtemp ---"

# 1. Compile the Device Tree Overlay
echo "Compiling nxp-simtemp.dtsi..."
dtc -O dtb -o ${WORKSPACE}/build/nxp-simtemp.dtbo -@ ${WORKSPACE}/kernel/dts/nxp-simtemp.dtsi
if [ $? -ne 0 ]; then
    echo "Error: Device Tree Overlay compilation failed."
    exit 1
fi
echo "nxp-simtemp.dtbo created."

# 2. Build Kernel Mode Driver (KMD)
echo "Compiling nxp-simtemp.c ..."
make -C ${WORKSPACE}/kernel 
if [ $? -ne 0 ]; then
    echo "Error: KMD compilation failed."
    exit 2
fi
echo "nxp-simtemp.ko created."

echo "--- Build complete ---"

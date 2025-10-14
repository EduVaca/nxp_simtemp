# nxp_simtemp - Simulated Temperature Sensor

This project demonstrates a simulated temperature sensor device driver for embedded Linux systems. It uses a Device Tree Overlay (DTO) to define a virtual device, a platform driver to manage it, and provides user-space applications for testing and visualization.

## Features

-   **Device Tree Overlay (`nxp-simtemp.dtsi`)**:
    -   Defines the `simtemp` device with initial properties for `sampling-ms` and `threshold-mC`.

## Prerequisites

### Host System

-   **Build Tools**: `gcc`, `make`, `dtc` (device-tree-compiler), etc.

    ```sh
    sudo apt-get update
    sudo apt-get install git build-essential fakeroot libncurses-dev libssl-dev ccache bison flex libelf-dev dwarves device-tree-compiler
    ```

-   **QEMU (for testing)**:

    ```sh
    sudo apt-get install qemu-system-aarch64 qemu-efi-aarch64
    ```

-   **Kernel headers for RBP image (QEMU ARM64)**:

    **TODO**: Automate Kernel headers setup to start building the KMD

    **ONLY for host architectures different than aarch64**: Get Linaro GCC binaries for cross compilation.

    ```sh
    cd /tmp

    # Get nearest GCC binaries used to build raspios_arm64-2020-05-28
    wget https://releases.linaro.org/components/toolchain/binaries/latest-5/aarch64-linux-gnu/gcc-linaro-5.5.0-2017.10-x86_64_aarch64-linux-gnu.tar.xz
    unxz gcc-linaro-5.5.0-2017.10-x86_64_aarch64-linux-gnu.tar.xz

    # Install Linaro 5
    sudo mkdir /usr/src/linaro-5
    sudo tar -xf gcc-linaro-5.5.0-2017.10-x86_64_aarch64-linux-gnu.tar -C /usr/src/linaro-5 --strip-components=1
    ```

    Get RBPI Linux headers

    ```sh
    cd /tmp

    # Get a stable RASPI-OS
    wget https://downloads.raspberrypi.com/raspios_arm64/images/raspios_arm64-2020-05-28/2020-05-27-raspios-buster-arm64.info

    # Grab the Kernel git hash used to build the kernel image, i.e., Kernel: https://github.com/raspberrypi/linux/tree/971a2bb14b459819db1bda8fcdf953e493242b42
    GIT_RBPI_HASH=$(cat 2020-05-27-raspios-buster-arm64.info | grep -oP 'Kernel:.*/tree/\K[0-9a-f]+')

    # Setup linux source code location
    sudo mkdir /usr/src/linux-5.4.42-v8+
    sudo chown --reference=${HOME} --recursive /usr/src/linux-5.4.42-v8+
    cd /usr/src/linux-5.4.42-v8+

    # Pull source code
    git init
    git remote add origin https://github.com/raspberrypi/linux
    git fetch --depth 1 origin ${GIT_RBPI_HASH}
    git checkout FETCH_HEAD

    # Set cross compilation
    export ARCH=arm64
    # If the running host is aarch64 set CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
    export CROSS_COMPILE=/usr/src/linaro-5/bin/aarch64-linux-gnu-

    # Configure the kernel for a RBPI target
    make -j$(nproc) bcmrpi3_defconfig

    # Rebuild external symbols
    make -j$(nproc) modules
    ```

## Setup and Installation

### 1. Build the Project

Use the provided `Makefile` to compile all components.

1.  **Run the script**:

    ```sh
    make
    ```

### 2. Deploy to the Target

#### For a Physical Embedded Board

1.  **Copy Files**: Transfer the following files to your target system.
    -   `nxp-simtemp.dtbo` -> `/boot/overlays/`
2.  **Enable the Overlay**: Add `dtoverlay=nxp-simtemp` to your bootloader configuration (e.g., `config.txt` for Raspberry Pi) and reboot.

#### For a QEMU ARM64 `raspi3b` Machine

1.  **Install QEMU for Ubuntu**:

    ```sh
    sudo apt update
    sudo apt install qemu-system-arm
    ```
2.  **Get a RBP ARM64 image**:

    ```sh
    # Create a temporal directory and move to it
    mkdir /tmp/work
    cd /tmp/work
    # Download a RBP ARM64 image from RPI repositories
    wget https://downloads.raspberrypi.com/raspios_arm64/images/raspios_arm64-2020-05-28/2020-05-27-raspios-buster-arm64.zip
    # Get the image
    unzip 2020-05-27-raspios-buster-arm64.zip 2020-05-27-raspios-buster-arm64.img
    ```

3.  **Prepare the image to boot with QEMU**:

    ```sh
    # Resize the image to the next power of 2 by inspecting the image file length and resizing it. I.e.,
    qemu-img info 2020-05-27-raspios-buster-arm64.img
    # Actual file length: 3.46 GiB (3711959040 bytes), next power of 2 is 4
    qemu-img resize 2020-05-27-raspios-buster-arm64.img "4G"
    ```
4.  **Extract the QEMU device tree and kernel to boot a RBP3 target**:

    ```sh
    # Get the start of the boot partition
    fdisk -l 2020-05-27-raspios-buster-arm64.img
    # Mount the boot partition (Start of img1 by block size = 8192 * 512)
    mkdir img1
    sudo mount -o loop,offset=4194304 ./2020-05-27-raspios-buster-arm64.img ./img1
    # Extract the kernel and device tree blob
    cp -v ./img1/bcm2710-rpi-3-b-plus.dtb .
    cp -v ./img1/kernel8.img .
    # Umount and clean
    sudo umount ./img1
    rm -r ./img1
    ```
5.  **Merge the DTB overlay with the base device tree**:

    ```sh
    fdtoverlay -o bcm2710-rpi-3-b-plus.dtb-merged.dtb -i bcm2710-rpi-3-b-plus.dtb nxp-simtemp.dtbo
    ```
6.  **Boot QEMU**: Use `virt-merged.dtb`.

    ```sh
    qqemu-system-aarch64 --machine raspi3b --cpu cortex-a53 --m 1024m --drive "format=raw,file=./2020-05-27-raspios-buster-arm64.img" -netdev user,id=net0,hostfwd=tcp::5022-:22 -device usb-net,netdev=net0 --dtb bcm2710-rpi-3-b-plus.dtb-merged.dtb --kernel kernel8.img --append "rw earlyprintk loglevel=8 console=ttyAMA0,115200 dwc_otg.lpm_enable=0 root=/dev/mmcblk0p2 rootwait panic=1 dwc_otg.fiq_fsm_enable=0" --no-reboot -device usb-mouse -device usb-kbd
    ```

## Usage

### 1. Validate the overlay is loaded at startup

Valdiate the folder `simtemp` is created under `/sys/firmware/devicetree/base`

-   **SSH to the system**:

    ```sh
    ssh -p 5022 pi@localhost
    ```
-   **Check the contents of `/sys/firmware/devicetree/base/simtemp`**:

    ```sh
    ls -l /sys/firmware/devicetree/base/simtemp
    ```

### 2. Validate KMD in QEMU ARM64

-   **Copy `nxp-simtemp.ko` to QEMU VM**:

    ```sh
    scp -P 5022 nxp-simtemp.ko pi@localhost:/tmp/
    ```

-   **Load the module using `insmod`**:

    ```sh
    # Double check the module has the correct fields
    modinfo nxp-simtemp.ko
    # Insert the module
    sudo insmod nxp-simtemp.ko
    ```

-   **Validate the module was loaded**:

    ```sh
    lsmod | grep nxp-simtemp
    # Look for simtemp samples
    sudo journalctl -b
    ```

-   **Unload the module**:

    ```sh
    rmmod nxp-simtemp
    # Validate it's no longer available
    lsmod | grep nxp-simtemp
    ```

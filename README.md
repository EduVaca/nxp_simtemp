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

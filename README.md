# Kernel mode driver (nxp_simtemp) for a simulated NXP temperature sensor

This project demonstrates a simulated temperature sensor device driver for embedded Linux systems. It uses a Device Tree Overlay (DTO) to define a virtual device, a platform driver to manage it, and provides user-space applications for testing and visualization.

## Features

-   **Device Tree Overlay (`nxp-simtemp.dtsi`)**:
    -   Defines the `simtemp` device with initial properties for `sampling-ms` and `threshold-mC`.

## Prerequisites

### Host System

-   **Build Tools**: `gcc`, `make`, `dtc` (device-tree-compiler), etc.

    ```sh
    sudo apt-get update
    sudo apt-get install git build-essential fakeroot libncurses-dev libssl-dev ccache bison flex \
    libelf-dev dwarves device-tree-compiler python3 cpplint pylint shellcheck unxz
    ```

-   **Kernel headers for Ubuntu host**:

    ```sh
    sudo apt install linux-headers-$(uname -r)
    ```

-   **QEMU (for VM testing using a Raspberry Pi 3B emulation - RBPI3B)**:

    ```sh
    sudo apt-get install qemu-system-aarch64 qemu-efi-aarch64
    ```

-   **Kernel headers for RBPI3B image (QEMU ARM64)**:

    **TODO**: Automate Kernel headers setup to start building the kernel mode driver and applications.

    **ONLY for host architectures different than aarch64**:

    Get Linaro GCC binaries for cross compilation. Other ARM64 architectures works without cross-compiling. Tested using Ubuntu 25.04 ARM64 and Ubuntu 24.04.3 AMD64.

    ```sh
    cd /tmp

    # Get the nearest GCC binaries used to build raspios_arm64-2020-05-28
    wget https://releases.linaro.org/components/toolchain/binaries/latest-5/aarch64-linux-gnu/gcc-linaro-5.5.0-2017.10-x86_64_aarch64-linux-gnu.tar.xz
    unxz gcc-linaro-5.5.0-2017.10-x86_64_aarch64-linux-gnu.tar.xz

    # Install Linaro 5
    sudo mkdir /usr/src/linaro-5
    sudo tar -xf gcc-linaro-5.5.0-2017.10-x86_64_aarch64-linux-gnu.tar -C /usr/src/linaro-5 --strip-components=1
    ```

    Get Raspberry Pi OS Linux headers

    ```sh
    cd /tmp

    # Get a stable RASPI-OS version
    wget https://downloads.raspberrypi.com/raspios_arm64/images/raspios_arm64-2020-05-28/2020-05-27-raspios-buster-arm64.info

    # Grab the Kernel git hash used to build the kernel image, i.e., Kernel: https://github.com/raspberrypi/linux/tree/971a2bb14b459819db1bda8fcdf953e493242b42
    GIT_RBPI_HASH=$(cat 2020-05-27-raspios-buster-arm64.info | grep -oP 'Kernel:.*/tree/\K[0-9a-f]+')

    # Setup linux source code location for module building
    sudo mkdir /usr/src/linux-5.4.42-v8+
    sudo chown --reference=${HOME} --recursive /usr/src/linux-5.4.42-v8+
    cd /usr/src/linux-5.4.42-v8+

    # Pull the source code that matches the downloaded Rasberry Pi OS image
    git init
    git remote add origin https://github.com/raspberrypi/linux
    git fetch --depth 1 origin ${GIT_RBPI_HASH}
    git checkout FETCH_HEAD

    # Set cross compilation
    export ARCH=arm64
    # If the running host is aarch64 set CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
    # Otherwie, point it to the just installed Linaro binaries
    export CROSS_COMPILE=/usr/src/linaro-5/bin/aarch64-linux-gnu-

    # Configure the kernel for a Raspberry Pi 3B+ target
    make -j$(nproc) bcmrpi3_defconfig

    # Rebuild external symbols before building out-of-tree new ones.
    make -j$(nproc) modules
    ```

## Setup and Installation

### 1. Build the Project

1.  **Clone the project**

    ```sh
    git clone https://github.com/EduVaca/nxp_simtemp.git
    cd nxp_simtemp
    ```

2.  **Run build the project**:

    For running host as target host:

    ```sh
    # Triger build process
    ./scripts/build.sh
    ```

    For RBPI3B as target host:

    ```sh
    # Set cross compilation
    export ARCH=arm64
    # If the running host is aarch64 set CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
    # Otherwie, point it to the just installed Linaro binaries
    export CROSS_COMPILE=/usr/src/linaro-5/bin/aarch64-linux-gnu-

    # Triger build process
    ./scripts/build.sh
    ```

    If the build finished corrrectly, it will create the following assets:

    ```sh
    .
    ├── build
    │   ├── nxp-simtemp.dtbo (Device Tree Blob Overlay)
    │   └── nxp_simtemp_test (Test user-space application)
    └── kernel
        ├── Module.symvers
        ├── modules.order
        ├── nxp_simtemp.ko (Kernel Mode driver)
        ├── nxp_simtemp.mod
        ├── nxp_simtemp.mod.c
        ├── nxp_simtemp.mod.o
        └── nxp_simtemp.o
    
    ```

    *NOTE:* To clean up the project run the following command: `./script/build.sh -c`

### 2. Deploy to target host

#### For an Ubuntu as the target host

1.  **Run `run_demo.sh` which validates the module is loaded, read/writen, and unload as expected**

    **Note:** If the target host was build with `CONFIG_MODULE_SIG` set to `y`, you will have to sign the kernel module `nxp_simtemo.ko`
    before loading or it may fail with `console: Key was rejected by service` & `dmesg: Loading of unsigned module is rejected`.

    For more information refer to `https://docs.kernel.org/admin-guide/module-signing.html`.

    ```sh
    sudo ./scripts/run_demo.sh
    ```

#### For a Physical Embedded Board (Not tested)

1.  **Copy Files**: Transfer the following files to your target system.
    -   `build/nxp-simtemp.dtbo` -> `/boot/overlays/`
2.  **Enable the Overlay**: Add `dtoverlay=nxp-simtemp` to your bootloader configuration (e.g., `config.txt` for Raspberry Pi) and reboot.

#### For a QEMU ARM64 `raspi3b` machine

1.  **Install QEMU for Ubuntu**:

    ```sh
    sudo apt update
    sudo apt install qemu-system-arm
    ```
2.  **Get a Raspberry Pi OS image (ARM64)**:

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
4.  **Extract the QEMU device tree and kernel to boot the RBP3 host**:

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
5.  **Merge the DTB overlay with the base DTB**:

    ```sh
    GIT_ROOT=/path/to/this/git/project
    cp -v ${GIT_ROOT}/build/nxp-simtemp.dtbo .
    fdtoverlay -o bcm2710-rpi-3-b-plus.dtb-merged.dtb -i bcm2710-rpi-3-b-plus.dtb nxp-simtemp.dtbo
    ```
6.  **Boot QEMU**: Use `virt-merged.dtb`.

    ```sh
    qqemu-system-aarch64 --machine raspi3b --cpu cortex-a53 --m 1024m --drive "format=raw,file=./2020-05-27-raspios-buster-arm64.img" -netdev user,id=net0,hostfwd=tcp::5022-:22 -device usb-net,netdev=net0 --dtb bcm2710-rpi-3-b-plus.dtb-merged.dtb --kernel kernel8.img --append "rw earlyprintk loglevel=8 console=ttyAMA0,115200 dwc_otg.lpm_enable=0 root=/dev/mmcblk0p2 rootwait panic=1 dwc_otg.fiq_fsm_enable=0" --no-reboot -device usb-mouse -device usb-kbd
    ```

    The above command will forward the host machine port `5022` to target host's port `22` for SSH connections. 

## Usage

#### For an Ubuntu host

-   **Validate the module is loaded**

    ```sh
    lsmod | grep nxp_simtemp
    ```

    If the module isn't loaded, load it running the following command (sign it before if required):

    ```sh
    sudo insmod kernel/nxp_simtemp.ko
    ```

-   **Run `setup.sh` as `root` to grant access for low privilage users**

    ```sh
    sudo ./scripts/setup.sh
    ```

-   **Run GUI demo**

    ```sh
    python3 ./user/gui/app.py
    ```

-   **Run CLI demo**

    ```sh
    # Get tool usage
    python3 ./user/cli/main.py
    ```

### For a QEMU ARM64 `raspi3b` Machine

#### 1. Validate the overlay is loaded at startup

Valdiate the folder `simtemp` is created under `/sys/firmware/devicetree/base`

-   **Enable SSH in the target host**:

    ```sh
    sudo systemctl enable ssh
    sudo systemctl start ssh
    # Validate ssh is running
    systemctl status ssh
    ```

-   **SSH to the system via the forwarded port**:

    ```sh
    ssh -p 5022 pi@localhost
    ```

-   **Check the contents of `/sys/firmware/devicetree/base/simtemp`**:

    ```sh
    ls -l /sys/firmware/devicetree/base/simtemp
    ```

#### 2. Validate module in QEMU ARM64

-   **Copy `nxp-simtemp.ko` to QEMU VM**:

    ```sh
    scp -P 5022 kernel/nxp-simtemp.ko pi@localhost:/tmp/
    ```

-   **SSH to the system via the forwarded port**:

    ```sh
    ssh -p 5022 pi@localhost
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

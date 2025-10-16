# Introduction

## 1.1 Purpose

  The purpose of this document is to outline the testing strategy for the embedded software project.
  The test plan covers building the project from a git repository, generating a host image for emulation,
  and testing a kernel-mode driver using a QEMU-emulated Raspberry Pi and Ubuntu 25.04 ARM64.

## 1.2 Objectives

  The main objectives are to:

  -  Ensure the project can be successfully downloaded and built on a development machine.
  -  Validate that a functional Raspberry Pi image can be created for QEMU emulation.
  -  Verify that the kernel-mode driver can be correctly inserted, tested, and unloaded in the running host and emulated environment.
  
## 2. Test scope

### 2.1 In scope

  Git Operations:
    
    Cloning, building, and compiling the project from its git repository.

  Image Generation:
  
    Creating a bootable Raspberry Pi image with all necessary dependencies for the QEMU environment.

  QEMU Emulation:
    
    Running the generated image inside QEMU and establishing network connectivity.

  Kernel Module Testing:
    
    Inserting, executing, and unloading the kernel-mode driver from within the QEMU guest environment.

### 2.2 Out of scope

  -  Testing on physical Raspberry Pi hardware.
  -  Stress or performance testing of the kernel module.
  -  Full functional or system-level testing of the entire embedded project.
  
### 3. Test strategy and approach

  The testing approach will follow a combination of unit, integration, and system-level testing.
  
  Unit Testing:
    
    Individual test cases for specific functions of the kernel module.
    
  Integration Testing:
  
    Verify the kernel module interacts correctly with the emulated Raspberry Pi OS.
    
  System Testing:
    
    Confirm that the entire workflow (build, emulate, test) functions as a complete system.

### 4. Test environment

### 4.1 Hardware

  Host Machine:
    
    A development machine with Ubuntu 25.04 ARM64 with a modern CPU, sufficient RAM, and ample storage for the git repository and QEMU image.

### 4.2 Software

  Git:
    
    Version control client to clone the project.
    
  Build Tools:
    
    GCC cross-compiler for the ARM architecture, make, and other necessary dependencies as specified by the project.
    
  QEMU:
    
    qemu-system-arm or qemu-system-aarch64 to emulate the Raspberry Pi hardware.
    
  Guest OS:
    
    A stripped-down Raspbian or other Linux distribution image suitable for the target Raspberry Pi architecture.
    
### 5. Test cases

### 5.1 Test case: Build from git

  Test Case ID:
    
    GIT-BUILD-001

  Description:
  
    Validate that the project can be successfully downloaded and compiled.
  
  Preconditions:
  
    Git is installed on the host machine.
  
  Steps:

    Clone the git repository using git clone [repository_url].
    Navigate to the project root directory.
    Execute the build command(s) (e.g., make).
  
  Expected Result:
  
    The project compiles without errors, and the necessary binary artifacts (including the kernel module) are produced.

### 5.2 Test case: Emulate Raspberry Pi

  Test Case ID:
  
    QEMU-RPI-001
    
  Description:
    
    Validate that a Raspberry Pi image can be emulated correctly in QEMU.
    
  Preconditions:

    Test case GIT-BUILD-001 has passed.
    QEMU is installed on the host machine.
    A compatible Raspberry Pi kernel and root filesystem image are available.

  Steps:
    
    Execute the QEMU command with the correct machine (-M) and kernel parameters (-kernel).
    Wait for the guest OS to boot.
    Use SSH to connect to the QEMU guest from the host machine.

  Expected Result:
    
    QEMU launches, the guest OS boots, and an SSH connection can be established successfully.

### 5.3 Test case: Insert kernel module

  Test Case ID:
    
    KMOD-INSERT-001
  
  Description:
    
    Verify the kernel-mode driver can be inserted into the emulated system.
    
  Preconditions:
    
    Test case QEMU-RPI-001 has passed.
    The kernel module binary is copied to the QEMU guest environment.
  
  Steps:
    
    SSH into the running QEMU guest.
    Run insmod [module_name.ko] with superuser permissions.
    Check the kernel log for any output using dmesg.
    
  Expected Result:
    
    The module inserts successfully without any errors, and the dmesg output contains the module's "Hello World" or equivalent log message.

### 5.4 Test case: Execute kernel module functions

  Test Case ID:
    
    KMOD-FUNC-001

  Description:
    
    Verify the core functionality of the kernel-mode driver.

  Preconditions:
    
    Test case KMOD-INSERT-001 has passed.
  
  Steps:
  
    Execute a test application or use a command-line tool to interact with the device created by the kernel module.
    Analyze the output of the test application and check the dmesg logs for module-specific messages.
  
  Expected Result:
    
    The test application receives the correct output, and the kernel log shows expected activity.

### 5.5 Test case: Unload kernel module

  Test Case ID:
    
    KMOD-UNLOAD-001

  Description:
    
    Verify the kernel-mode driver can be safely unloaded from the system.
  
  Preconditions:
  
    Test case KMOD-INSERT-001 has passed.

  Steps:
    
    Execute rmmod [module_name] with superuser permissions.
    Check the kernel log for any output using dmesg.

  Expected Result:
    
    The module unloads successfully, and the dmesg output contains the module's "Goodbye" or equivalent log message, confirming successful cleanup.
    
### 6. Roles and responsibilities

  Test Manager:
    
    Responsible for test plan creation, strategy, and oversight.
    
  Test Engineer:
    
    Performs test execution, reports defects, and documents test results.
    
  Developer:
    
    Supports the test team, provides assistance, and fixes identified bugs.
    
### 7. Risk analysis

  Risk:
    
    The git project fails to build due to missing dependencies or toolchain issues.
    
  Mitigation:
    
    Verify all project dependencies and cross-compilation toolchains are installed and correctly configured before starting.
    
  Risk:
    
    QEMU emulation fails or is unstable, preventing access to the guest OS.
    
  Mitigation:
  
    Use a known-working QEMU kernel and image combination. Ensure QEMU command-line parameters are correctly configured.
    
  Risk:
    
    The kernel module fails to insert or causes a kernel panic.
    
  Mitigation:
    
    Provide detailed logging in the kernel module code to debug failures. Enable the QEMU gdbstub for low-level kernel debugging.

### 9. Deliverables
  
-  Test Plan Document
-  Test Case Specifications
-  Test Summary Report
-  Defect Reports
-  Automated Test Scripts

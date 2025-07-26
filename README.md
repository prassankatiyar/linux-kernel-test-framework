# Automated Linux Kernel Module Testing Framework

This project is a complete, automated framework for compiling, loading, testing, and unloading a custom Linux kernel module. It demonstrates a full CI/CD-style validation loop for kernel-space software development.

## Core Components

1.  **Kernel Module (`mydevice.c`)**: A character device driver written in C that exposes `read`, `write`, and `ioctl` functionalities for managing device state.
2.  **Test Suite (`test_suite.c`)**: A user-space C program that performs unit tests on the driver to validate I/O integrity, error handling (e.g., buffer overflows), and state transitions via `ioctl`.
3.  **Automation Framework (`run_tests.py`)**: A Python script that orchestrates the entire testing process:
    - Cleans the environment.
    - Compiles both the kernel module and the test suite.
    - Loads the module using `insmod`.
    - Sets correct permissions on the `/dev` file.
    - Executes the test suite and parses its results.
    - Unloads the module using `rmmod`.
    - Provides a clean, color-coded summary of the results.

## How to Run

1.  Ensure you have the necessary build tools and kernel headers:
    `sudo apt install build-essential linux-headers-$(uname -r)`
2.  Run the automation script:
    `python3 run_tests.py`

## Technologies Used
- **Languages**: C, Python, Bash
- **Tools**: Makefiles, Git, GCC
- **Kernel APIs**: Character Devices, `ioctl`, `insmod`, `rmmod`

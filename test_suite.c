// test_suite.c
// A user-space program to test the mydevice driver.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // For open()
#include <unistd.h>     // For read(), write(), close()
#include <sys/ioctl.h>  // For ioctl()
#include <errno.h>      // For errno

// Include the shared header to ensure API consistency.
#include "mydevice.h"

#define DEVICE_PATH "/dev/mydevice"
#define TEST_STRING "Hello, Kernel!"
// This should match the buffer size in your kernel module
#define KERNEL_BUFFER_SIZE 256 

// --- Helper function for clear reporting ---
void run_test(const char* test_name, int condition) {
    printf("%-50s ", test_name);
    if (condition) {
        printf("[\x1B[32mPASS\x1B[0m]\n");
    } else {
        printf("[\x1B[31mFAIL\x1B[0m]\n");
        // Exit on failure to prevent cascading errors and get a clear failure point.
        exit(EXIT_FAILURE); 
    }
}


int main() {
    int fd;
    int status;
    int ret; // Variable to hold return values of system calls
    ssize_t bytes_written, bytes_read;

    char read_buffer[KERNEL_BUFFER_SIZE];
    char large_buffer[KERNEL_BUFFER_SIZE + 100];

    printf("--- Starting mydevice Test Suite ---\n");

    // --- Test 1: Open the device file ---
    fd = open(DEVICE_PATH, O_RDWR);
    run_test("1. Opening the device file", (fd >= 0));
    if (fd < 0) {
        perror("   Error details");
        return EXIT_FAILURE;
    }

    // --- Test 2: Basic Write and Read ---
    printf("\n--- Testing Basic I/O ---\n");
    bytes_written = write(fd, TEST_STRING, strlen(TEST_STRING));
    run_test("2. Writing test string to device", (bytes_written == strlen(TEST_STRING)));

    memset(read_buffer, 0, sizeof(read_buffer));
    bytes_read = read(fd, read_buffer, strlen(TEST_STRING));
    run_test("3. Reading test string from device", (bytes_read == strlen(TEST_STRING)));

    run_test("4. Verifying read content matches written content", (strcmp(TEST_STRING, read_buffer) == 0));
    if (strcmp(TEST_STRING, read_buffer) != 0) {
        printf("   - Expected: '%s'\n", TEST_STRING);
        printf("   - Got:      '%s'\n", read_buffer);
    }

    // --- Test 3: IOCTL Commands ---
    printf("\n--- Testing IOCTL Commands ---\n");
    
    // Set status to a garbage value to ensure ioctl is writing to it.
    status = -1; 
    ret = ioctl(fd, MYDEVICE_IOCTL_GET_STATUS, &status);
    run_test("5. IOCTL GET_STATUS (after write)", (ret == 0 && status == strlen(TEST_STRING)));
    if (ret != 0) perror("   ioctl(GET_STATUS) failed");

    // This test now correctly checks the return value of the ioctl call.
    ret = ioctl(fd, MYDEVICE_IOCTL_RESET, NULL);
    run_test("6. IOCTL RESET (clearing the device buffer)", (ret == 0));
    if (ret != 0) perror("   ioctl(RESET) failed");

    // Reset status again to ensure the next call is tested properly.
    status = -1; 
    ret = ioctl(fd, MYDEVICE_IOCTL_GET_STATUS, &status);
    run_test("7. IOCTL GET_STATUS (after reset)", (ret == 0 && status == 0));
    if (ret != 0) perror("   ioctl(GET_STATUS) after reset failed");

    // --- Test 4: Edge Case - Writing too much data ---
    printf("\n--- Testing Edge Cases ---\n");
    memset(large_buffer, 'X', sizeof(large_buffer));
    
    bytes_written = write(fd, large_buffer, sizeof(large_buffer));
    run_test("8. Writing more than buffer size (error check)", (bytes_written < 0 && errno == EFAULT));
    if (bytes_written >= 0) {
        printf("   - Note: Driver did not return an error on overflow.\n");
    } else {
        // We expect an error, so printing perror here is for information.
        perror("   - Expected error on overflow");
    }

    // --- Cleanup ---
    close(fd);
    printf("\n--- Test Suite Finished ---\n");
    printf("All tests passed!\n");

    return EXIT_SUCCESS;
}


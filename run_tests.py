#!/usr/bin/env python3
import subprocess
import sys

# --- Configuration ---
MODULE_NAME = "mydevice"
TEST_SUITE_EXEC = "./test_suite"
# --- End Configuration ---

# --- Color Codes for Output ---
COLOR_GREEN = '\033[92m'
COLOR_RED = '\033[91m'
COLOR_YELLOW = '\033[93m'
COLOR_BLUE = '\033[94m'
COLOR_RESET = '\033[0m'

def print_status(message, status):
    """Prints a message with a colored PASSED or FAILED status."""
    status_text = f"{COLOR_GREEN}PASSED{COLOR_RESET}" if status else f"{COLOR_RED}FAILED{COLOR_RESET}"
    print(f"{message:<50} [{status_text}]")

def run_command(command, sudo=False, capture=False, check=True):
    """Runs a command and handles errors."""
    if sudo and not sys.platform.startswith("linux"):
        print("Sudo is only supported on Linux.")
        sys.exit(1)

    cmd_list = ["sudo"] + command if sudo else command
    
    try:
        # Using subprocess.run is more modern and flexible
        result = subprocess.run(
            cmd_list, 
            capture_output=capture, 
            text=True, 
            check=check
        )
        return result
    except FileNotFoundError:
        print_status(f"Command not found: {cmd_list[0]}", False)
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        print_status(f"Error running command: {' '.join(cmd_list)}", False)
        if e.stdout:
            print(f"--- STDOUT ---\n{e.stdout}")
        if e.stderr:
            print(f"--- STDERR ---\n{e.stderr}")
        sys.exit(1)
    except Exception as e:
        print_status(f"An unexpected error occurred: {e}", False)
        sys.exit(1)

def initial_cleanup():
    """
    Tries to unload the module before starting, in case it was
    left loaded from a previous failed run.
    """
    print(f"\n{COLOR_BLUE}--- Pre-flight Check: Cleaning up old modules ---{COLOR_RESET}")
    #check=False because we EXPECT this to fail if the module isn't loaded.

    run_command(["rmmod", MODULE_NAME], sudo=True, check=False)
    print("Cleanup check complete.")


def compile_code():
    """Compiles the kernel module and the test suite."""
    print(f"\n{COLOR_BLUE}--- Phase 1: Compiling Code ---{COLOR_RESET}")
    try:
        run_command(["make", "clean"])
        run_command(["make"])
        print_status("Compilation of module and test suite", True)
        return True
    except Exception:
        return False

def load_module():
    """Loads the kernel module, verifies it, and sets permissions."""
    print(f"\n{COLOR_BLUE}--- Phase 2: Loading Kernel Module ---{COLOR_RESET}")
    try:
        run_command(["insmod", f"{MODULE_NAME}.ko"], sudo=True)
        print_status(f"Loading module '{MODULE_NAME}.ko'", True)

        # Verify module is loaded
        result = run_command(["lsmod"], capture=True)
        if MODULE_NAME in result.stdout:
            print_status(f"Verification with 'lsmod'", True)
            
      
            # Set permissions on the device file so the user-space test
  
            try:
                device_path = f"/dev/{MODULE_NAME}"
                run_command(["chmod", "666", device_path], sudo=True)
                print_status(f"Setting permissions on {device_path}", True)
            except Exception as e:
                print_status(f"Setting permissions on {device_path}", False)
                print(f"{COLOR_RED}Could not set permissions. The test suite will likely fail.{COLOR_RESET}")
            
            return True
        else:
            print_status(f"Verification with 'lsmod'", False)
            return False
    except Exception:
        return False

def run_test_suite():
    """Runs the user-space test suite."""
    print(f"\n{COLOR_BLUE}--- Phase 3: Running Test Suite ---{COLOR_RESET}")
    try:
        result = run_command([TEST_SUITE_EXEC], capture=True, check=False) # check=False to see test output
        
        print("--- Test Application Output ---")
        # Only print stdout if it exists
        if result.stdout:
            print(result.stdout.strip())
        print("-----------------------------")

        # A simple check: "All tests passed!" 
        if result.returncode == 0 and "All tests passed!" in result.stdout:
            print_status("Test suite execution", True)
            return True
        else:
            print_status("Test suite execution", False)
            if result.stderr:
                print(f"{COLOR_RED}Test suite errors:\n{result.stderr.strip()}{COLOR_RESET}")
            return False
    except Exception:
        return False

def final_cleanup():
    """Unloads the kernel module at the end."""
    print(f"\n{COLOR_BLUE}--- Phase 4: Final Cleanup ---{COLOR_RESET}")
    try:
        # Use check=False as the module might already be gone if tests failed
        run_command(["rmmod", MODULE_NAME], sudo=True, check=False)
        print_status(f"Unloading module '{MODULE_NAME}'", True)
        run_command(["make", "clean"])
        print_status("Cleaning build files", True)
        return True
    except Exception:
        return False

def main():
    """Main function to run the entire workflow."""
    print(f"{COLOR_YELLOW}===== Starting Automated Kernel Module Test Framework ====={COLOR_RESET}")

    initial_cleanup()

    if not compile_code():
        print(f"\n{COLOR_RED}Workflow stopped due to compilation failure.{COLOR_RESET}")
        sys.exit(1)
    
    if not load_module():
        print(f"\n{COLOR_RED}Workflow stopped due to module loading failure.{COLOR_RESET}")
        final_cleanup()
        sys.exit(1)

    tests_passed = run_test_suite()

    # Final Summary
    print(f"\n{COLOR_YELLOW}================== TEST SUMMARY =================={COLOR_RESET}")
    if tests_passed:
        print(f"{COLOR_GREEN}✅ All tests PASSED! The framework ran successfully.{COLOR_RESET}")
    else:
        print(f"{COLOR_RED}❌ Some tests FAILED. Please review the output above.{COLOR_RESET}")
    
    print(f"{COLOR_YELLOW}=================================================={COLOR_RESET}")

    final_cleanup()
    print("\nFramework finished.")

if __name__ == "__main__":
    main()


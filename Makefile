
ifneq ($(KERNELRELEASE),)
    # This is the kbuild part of the Makefile.
    # It tells kbuild to build 'mydevice.o' into a loadable module 'mydevice.ko'.
    obj-m := mydevice.o

# This 'else' block is processed by the user's initial 'make' command.
else
    # Define the path to the kernel build directory.
    KDIR := /lib/modules/$(shell uname -r)/build
    # Get the current working directory.
    PWD := $(shell pwd)

# The default target, 'all', depends on 'modules' and 'test_suite'.
all: modules test_suite

# 'modules' is the target to build.
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# The 'test_suite' target compiles our user-space C program.
# This is a standard compilation rule.
test_suite: test_suite.c mydevice.h
	$(CC) test_suite.c -o test_suite

# The 'clean' target cleans up all compiled files.

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_suite


install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

endif

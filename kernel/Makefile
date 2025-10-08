# TODO: Since the compilation is intended for cross-platform support on ARM,
#       KROOT is going to be a different kernel version than the one
#       in the build machine.
KROOT := /lib/modules/$(shell uname -r)/build
SRC := $(shell pwd)

obj-m := nxp_simtemp.o

all: modules

modules:
	@$(MAKE) -C $(KROOT) M=$(SRC) modules

kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(SRC) clean

clean: kernel_clean
	rm -rf Module.symvers modules.order
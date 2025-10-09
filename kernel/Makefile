# TODO: Since the compilation is intended for cross-platform support on ARM,
#       KROOT is going to be a different kernel version than the one
#       in the build machine.
#KROOT := /lib/modules/$(shell uname -r)/build
KROOT := /usr/src/linux-5.4.42-v8+
SRC := $(shell pwd)
EXTRA_CFLAGS := -Wall -Wstrict-prototypes -Wmissing-prototypes
INST_DIR := extra
INST_PATH := $(SRC)/../build

all: modules

modules:
	@$(MAKE) -C $(KROOT) M=$(SRC) modules

modules_install:
	@$(MAKE) INSTALL_MOD_PATH=$(INST_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(INST_DIR) -C $(KROOT) M=$(SRC) modules_install

kernel_clean:
	@$(MAKE) INSTALL_MOD_PATH=$(INST_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(INST_DIR) -C $(KROOT) M=$(SRC) clean

clean: kernel_clean
	rm -rf Module.symvers modules.order
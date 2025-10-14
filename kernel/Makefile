# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

SRC := $(shell pwd)
BUILD_DIR := $(SRC)/../build
EXTRA_CFLAGS := -Wall -Wstrict-prototypes -Wmissing-prototypes
INST_DIR := extra
GCC := $(CROSS_COMPILE)gcc

ifneq ($(CROSS_COMPILE),)
	KROOT := /usr/src/linux-5.4.42-v8+
	INST_PATH := $(SRC)/../build
	CFLAGS_MODULE := -DRBPITGT
else
	KROOT := /lib/modules/$(shell uname -r)/build
endif


all: modules test

test:
	$(GCC) $(EXTRA_CFLAGS) -o $(BUILD_DIR)/nxp_simtemp_test nxp_simtemp_test.c

modules:
	@$(MAKE) CFLAGS_MODULE=$(CFLAGS_MODULE) -C $(KROOT) M=$(SRC) modules

modules_install:
	@$(MAKE) INSTALL_MOD_PATH=$(INST_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(INST_DIR) -C $(KROOT) M=$(SRC) modules_install

kernel_clean:
	@$(MAKE) INSTALL_MOD_PATH=$(INST_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(INST_DIR) -C $(KROOT) M=$(SRC) clean

clean: kernel_clean
	rm -rf Module.symvers modules.order $(BUILD_DIR)/nxp_simtemp_test
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

KROOT := $(KERNEL_HEADERS)
SRC := $(shell pwd)
EXTRA_CFLAGS := -Wall -Wstrict-prototypes -Wmissing-prototypes
INST_DIR := extra
GCC := $(CROSS_COMPILE)gcc

ifneq ($(CROSS_COMPILE),)
	L_INSTALL_MODE_PATH := $(BUILD_PATH)
	L_MODE_DIR := $(BUILD_PATH)
	CFLAGS_MODULE := -DRBPITGT
else
	L_MODE_DIR := /lib/modules/$(shell uname -r)
endif



all: modules test

test:
	@echo $(GCC) $(EXTRA_CFLAGS) -o $(BUILD_PATH)/nxp_simtemp_test nxp_simtemp_test.c
	@$(GCC) $(EXTRA_CFLAGS) -o $(BUILD_PATH)/nxp_simtemp_test nxp_simtemp_test.c

modules:
	@echo $(MAKE) CFLAGS_MODULE=$(CFLAGS_MODULE) -C $(KROOT) M=$(SRC) modules
	@$(MAKE) CFLAGS_MODULE=$(CFLAGS_MODULE) -C $(KROOT) M=$(SRC) modules

modules_install:
	@echo $(MAKE) INSTALL_MOD_PATH=$(L_INSTALL_MODE_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(L_MODE_DIR) -C $(KROOT) M=$(SRC) modules_install
	@$(MAKE) INSTALL_MOD_PATH=$(L_INSTALL_MODE_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(L_MODE_DIR) -C $(KROOT) M=$(SRC) modules_install

kernel_clean:
	@echo $(MAKE) INSTALL_MOD_PATH=$(L_INSTALL_MODE_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(L_MODE_DIR) -C $(KROOT) M=$(SRC) clean
	@$(MAKE) INSTALL_MOD_PATH=$(L_INSTALL_MODE_PATH) INSTALL_MOD_DIR=$(INST_DIR) MOD_DIR=$(L_MODE_DIR) -C $(KROOT) M=$(SRC) clean

clean: kernel_clean
	rm -rf Module.symvers modules.order $(BUILD_PATH)/nxp_simtemp_test
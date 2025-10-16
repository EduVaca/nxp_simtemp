/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nxp_simtemp_test.h - Header file for user space testing application to
 *                      validate a kernel mode driver simulating a temperature
*                       sensor.
 *
 * Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>
 *
 * See README.md for more information.
 */

#ifndef KERNEL_INCLUDE_NXP_SIMTEMP_TEST_H_
#define KERNEL_INCLUDE_NXP_SIMTEMP_TEST_H_

#include <linux/types.h>

/* --- Prototypes --- */
void ns_to_iso8601(__u64 ns, char* buffer, size_t size);
void print_help(char *prog_name);

#endif  // KERNEL_INCLUDE_NXP_SIMTEMP_TEST_H_

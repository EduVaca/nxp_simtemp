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

#include <linux/types.h>

/* --- Prototypes --- */
void ns_to_iso8601(long long ns, char* buffer, size_t size);
void print_help(char *prog_name);

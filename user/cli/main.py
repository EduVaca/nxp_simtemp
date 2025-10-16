#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

""" Wrapper to testing application

This program bypassed all parameters to the testing application

Usage: python3 main.py [options]
Options:
  -s <ms>           Set sampling period via sysfs.
  -t <mC>           Set threshold via sysfs.
  -m <mode>         Set mode via sysfs (normal|ramp).
  -i <ms>:<mC>:<mode>  Set all via ioctl (mode: 0=normal, 1=ramp).
  -p                Run in poll loop, printing samples and alerts.

Resources:

"""

###############################################################################################

import subprocess
from pathlib import Path
import sys
import os

###############################################################################################


if __name__ == '__main__':
    # Main program

    EXIT_CODE = 1
    # Get to the root folder
    path_to_program = Path(__file__).resolve().parent.parent.parent
    # Build the absolute program path
    program = os.path.join(path_to_program, "build", "nxp_simtemp_test")
    # Bypass all arguments
    command = [program] + sys.argv[1:]
    try:
        # Execute and get the return code
        result = subprocess.run(command, check=False)
        EXIT_CODE = result.returncode
    except FileNotFoundError:
        print(f"Error: Program '{program}' not found.")
    except KeyboardInterrupt:
        pass

    # Exit with same error as program
    sys.exit(EXIT_CODE)

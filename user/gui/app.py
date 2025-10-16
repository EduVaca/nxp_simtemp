#!/usr/bin/env python
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>

""" Temperature gauge

This Python 3 application uses Tkinter to create a temperature gauge.

Temperature gauge provides following features:

    * It reads binary data from a simulated character device /dev/simtemp and
      adjusts device parameters by writing to SysFS files in
      /sys/devices/platform/.

    * The application uses multithreading to read from the device without
      freezing the GUI and includes an animated gauge widget to visualize
      the temperature.

Usage:

    * Gauge
        python3 app.py

Resources:

"""

################################################################################

import tkinter as tk
from tkinter import ttk
from threading import Thread, Event
import struct
from enum import Enum
import time
import math

################################################################################

# Configuration and file paths
DEV = "simtemp"
DEV_TEMP = "/dev/" + DEV
SYSFS_SAMPLING_MS = "/sys/devices/platform/" + DEV + "/sampling_ms"
SYSFS_THRESHOLD_MC = "/sys/devices/platform/" + DEV + "/threshold_mC"
SYSFS_MODE = "/sys/devices/platform/" + DEV + "/mode"

# Data structure definition for the binary data from the device
# We use '<QIHx' for little-endian:
# Q: __u64 (unsigned long long) for timestamp_ns
# I: __u32 (unsigned int) for temp_mc
# H: __u16 (unsigned short) for flags
# H: __u16 padding
SIMTEMP_FORMAT = '<QIHH'
SIMTEMP_SIZE = struct.calcsize(SIMTEMP_FORMAT)

# Alerting flags
FLAG_NEW_SAMPLE = 0b01
FLAG_THRESHOLD_CROSSED = 0b10

################################################################################

class Label(Enum):
    """
    Enum class for labeled texts
    """
    CURRENT_TEMP = 1
    ALERT_STATUS = 2
    SAMPLING_MS = 3
    THRESHOLD_MC = 4
    MODE = 5


class TempGaugeApp:
    """
    Main application class for the temperature gauge.
    """
    def __init__(self, master):
        self.master = master
        master.title("Temperature Gauge")

        self.gauge_needle = None
        self.threshold_mc = None
        self.labels = {}

        # Variables for GUI elements
        self.labels[Label.CURRENT_TEMP] = tk.StringVar(value="--.- °C")
        self.labels[Label.ALERT_STATUS] = tk.StringVar(value="Normal")
        self.labels[Label.SAMPLING_MS] = tk.StringVar()
        self.labels[Label.THRESHOLD_MC] = tk.StringVar()
        self.labels[Label.MODE] = tk.StringVar()

        # Load initial values from SysFS
        self.load_sysfs_values()

        # Initialize the GUI
        self.create_widgets()

        # Start the background thread for reading temperature
        self.stop_event = Event()
        Thread(target=self.read_temp_device, daemon=True).start()

    def create_widgets(self):
        """
        Creates all the Tkinter widgets for the application.
        """
        main_frame = ttk.Frame(self.master, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)

        # Gauge visualization area (using a Canvas)
        self.gauge_canvas = tk.Canvas(
            main_frame,
            width=300,
            height=200,
            bg='white'
        )
        self.gauge_canvas.pack(pady=10)
        self.draw_gauge_background()

        # Current temperature display
        temp_label_frame = ttk.LabelFrame(
            main_frame,
            text="Current Temperature"
        )
        temp_label_frame.pack(fill=tk.X, pady=5)
        ttk.Label(
            temp_label_frame,
            textvariable=self.labels[Label.CURRENT_TEMP],
            font=("Helvetica", 24)
        ).pack(pady=5)

        # Alert status display
        alert_frame = ttk.LabelFrame(main_frame, text="Alert Status")
        alert_frame.pack(fill=tk.X, pady=5)
        self.alert_label = ttk.Label(
            alert_frame,
            textvariable=self.labels[Label.ALERT_STATUS],
            font=("Helvetica", 14),
            foreground='green'
        )
        self.alert_label.pack(pady=5)

        # Control fields (Sampling time, Threshold, Mode)
        controls_frame = ttk.LabelFrame(main_frame, text="Device Controls")
        controls_frame.pack(fill=tk.X, pady=5)

        # Sampling time control
        ttk.Label(
            controls_frame,
            text="Sampling (ms):"
        ).grid(row=0, column=0, sticky=tk.W, padx=5, pady=2)
        ttk.Entry(
            controls_frame,
            textvariable=self.labels[Label.SAMPLING_MS]
        ).grid(row=0, column=1, padx=5, pady=2)
        ttk.Button(
            controls_frame,
            text="Set",
            command=self.set_sampling_ms
        ).grid(row=0, column=2, padx=5, pady=2)

        # Threshold control
        ttk.Label(
            controls_frame,
            text="Threshold (°C):"
        ).grid(row=1, column=0, sticky=tk.W, padx=5, pady=2)
        ttk.Entry(
            controls_frame,
            textvariable=self.labels[Label.THRESHOLD_MC]
        ).grid(row=1, column=1, padx=5, pady=2)
        ttk.Button(
            controls_frame,
            text="Set",
            command=self.set_threshold_mc
        ).grid(row=1, column=2, padx=5, pady=2)

        # Operation mode control
        ttk.Label(
            controls_frame,
            text="Mode:"
        ).grid(row=2, column=0, sticky=tk.W, padx=5, pady=2)
        mode_combobox = ttk.Combobox(
            controls_frame,
            textvariable=self.labels[Label.MODE],
            values=["normal", "ramp"],
            state="readonly"
        )
        mode_combobox.grid(row=2, column=1, padx=5, pady=2)
        ttk.Button(
            controls_frame,
            text="Set",
            command=self.set_mode
        ).grid(row=2, column=2, padx=5, pady=2)

    def draw_gauge_background(self):
        """
        Draws the static background elements of the gauge.
        """
        self.gauge_canvas.delete(tk.ALL)
        self.gauge_canvas.create_arc(
            10,
            10,
            290,
            290,
            start=0, extent=180, outline='gray', width=10, style=tk.ARC)

        min_temp = 0
        max_temp = self.threshold_mc / 1000.0
        num_labels = 11  # Number of labels for the gauge
        temp_interval = (max_temp - min_temp) / (num_labels - 1)

        for i in range(num_labels):
            # Adjust angle calculation
            angle = 180 - i * (180 / (num_labels - 1))
            rad = angle * 3.14159 / 180
            x1 = 150 + 120 * math.cos(rad)
            y1 = 150 - 120 * math.sin(rad)
            x2 = 150 + 140 * math.cos(rad)
            y2 = 150 - 140 * math.sin(rad)
            self.gauge_canvas.create_line(x1, y1, x2, y2, fill='gray', width=2)
            label_text = f"{min_temp + i * temp_interval:.0f}"
            self.gauge_canvas.create_text(
                    x1,
                    y1,
                    text=label_text, anchor='center', font=('Arial', 8))
        self.gauge_needle = self.gauge_canvas.create_line(
            150, 150, 150, 20, fill='red', width=4
        )
        self.gauge_canvas.create_text(
            150, 180, text="Temperature (°C)", font=("Arial", 10, "bold"))

    def update_gauge_and_display(self, temp_mc, flags):
        """
        Updates the gauge needle position and temperature display.
        """
        temp_c = temp_mc / 1000.0
        self.labels[Label.CURRENT_TEMP].set(f"{temp_c:.1f} °C")

        # Map temperature to gauge angle
        temp_range = self.threshold_mc / 1000.0
        temp_offset = 0
        angle = ((temp_c - temp_offset) / temp_range) * 180
        angle = max(angle, 0)
        angle = min(angle, 180)

        # Calculate new needle coordinates
        rad = (180 - angle) * 3.14159 / 180
        x = 150 + 130 * math.cos(rad)
        y = 150 - 130 * math.sin(rad)

        # Update gauge needle
        self.gauge_canvas.coords(self.gauge_needle, 150, 150, x, y)

        # Update alert status
        if flags & FLAG_THRESHOLD_CROSSED:
            self.labels[Label.ALERT_STATUS].set("ALERT: Threshold Crossed!")
            self.alert_label.config(foreground='red')
        else:
            self.labels[Label.ALERT_STATUS].set("Normal")
            self.alert_label.config(foreground='green')

    def read_temp_device(self):
        """
        Background thread function to read data from the character device.
        """
        try:
            with open(DEV_TEMP, "rb") as f:
                while not self.stop_event.is_set():
                    try:
                        data = f.read(SIMTEMP_SIZE)
                        if not data:
                            time.sleep(0.1) # Wait if no data
                            continue

                        _, temp_mc, flags, _ = struct.unpack(
                            SIMTEMP_FORMAT, data
                        )

                        # Use master.after() to update GUI from the main thread
                        self.master.after(
                            0,
                            self.update_gauge_and_display,
                            temp_mc,
                            flags)
                    except (IOError, struct.error) as e:
                        print(f"Error reading or unpacking from device: {e}")
                        time.sleep(1)
        except FileNotFoundError:
            print(f"Error: Character device {DEV_TEMP} not found.")
            self.master.after(
                0,
                self.labels[Label.ALERT_STATUS].set,
                "Device not found!"
            )

    def load_sysfs_values(self):
        """
        Reads initial values from SysFS files.
        """
        try:
            with open(SYSFS_SAMPLING_MS, "r", encoding="utf-8") as f:
                self.labels[Label.SAMPLING_MS].set(f.read().strip())
            with open(SYSFS_THRESHOLD_MC, "r", encoding="utf-8") as f:
                self.threshold_mc = int(f.read().strip())
                self.labels[Label.THRESHOLD_MC].set(
                    f"{self.threshold_mc / 1000.0:.1f}"
                )
            with open(SYSFS_MODE, "r", encoding="utf-8") as f:
                self.labels[Label.MODE].set(f.read().strip())
        except (FileNotFoundError, IOError) as e:
            print(f"Warning: Could not read SysFS files. {e}")
            self.labels[Label.SAMPLING_MS].set("N/A")
            self.labels[Label.THRESHOLD_MC].set("N/A")
            self.labels[Label.MODE].set("N/A")

    def write_to_sysfs(self, path, value):
        """
        Helper function to write a value to a SysFS file.
        """
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(str(value))
        except (FileNotFoundError, IOError) as e:
            print(f"Error: Could not write to {path}. Check permissions. {e}")

    def set_sampling_ms(self):
        """
        Writes the new sampling time to SysFS.
        """
        try:
            value = int(self.labels[Label.SAMPLING_MS].get())
            self.write_to_sysfs(SYSFS_SAMPLING_MS, value)
            with open(SYSFS_SAMPLING_MS, "r", encoding="utf-8") as f:
                self.labels[Label.SAMPLING_MS].set(f.read().strip())
        except ValueError:
            print("Invalid value for sampling time. Please enter an integer.")

    def set_threshold_mc(self):
        """
        Writes the new temperature threshold to SysFS.
        """
        try:
            value = int(float(self.labels[Label.THRESHOLD_MC].get()) * 1000)
            self.write_to_sysfs(SYSFS_THRESHOLD_MC, value)
            with open(SYSFS_THRESHOLD_MC, "r", encoding="utf-8") as f:
                self.threshold_mc = int(f.read().strip())
                self.labels[Label.THRESHOLD_MC].set(
                    f"{self.threshold_mc / 1000.0:.1f}"
                )
            self.draw_gauge_background()
        except ValueError:
            print("Invalid value for threshold. Please enter a number.")

    def set_mode(self):
        """
        Writes the new operation mode to SysFS.
        """
        value = self.labels[Label.MODE].get()
        if value in ["normal", "ramp"]:
            self.write_to_sysfs(SYSFS_MODE, value)
            with open(SYSFS_MODE, "r", encoding="utf-8") as f:
                self.labels[Label.MODE].set(f.read().strip())
        else:
            print("Invalid mode. Must be 'normal' or 'ramp'.")

# Main application entry point
if __name__ == "__main__":
    root = tk.Tk()
    app = TempGaugeApp(root)
    root.protocol(
        "WM_DELETE_WINDOW",
        lambda: (app.stop_event.set(), root.destroy())
    )
    root.mainloop()

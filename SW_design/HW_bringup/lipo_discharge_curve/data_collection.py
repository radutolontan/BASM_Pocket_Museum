from saleae import automation
import os
import os.path
import csv
import re
import shutil
import time
import serial
import matplotlib.pyplot as plt
from datetime import datetime

DEVICE_ID = 'ECCE3A142D5FF009'
ANALOG_CHANNELS = [4, 5]
SAMPLE_RATE = 50            # Sa/s
CAPTURE_DURATION_S = 0.5    # seconds per capture

SERIAL_PORT = '/dev/ttyACM0'
SERIAL_BAUDRATE = 115200    # adjust if your firmware uses a different rate
SERIAL_READ_TIMEOUT_S = 1   # how long each ser.readline() call blocks at most
IDLE_TIMEOUT_S = 30         # stop the program if no valid message arrives within this window

# Matches lines like: "p-l-vbat: 12.345,11.987"
VBAT_LINE_RE = re.compile(r"^p-l-vbat:\s*(-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?)\s*$")

# Current / capacity computation parameters
OFFSET = 0.0          # V, sensor offset
R = 1.0               # ohms, shunt resistance
CAPACITY_TOTAL_MAH = 500.0

# Where to stage exports temporarily before deleting them. Reused every
# iteration so we don't leave thousands of folders behind.
TEMP_EXPORT_DIR = os.path.join(os.getcwd(), "capture_export_tmp")

# Persistent log file that accumulates results across all triggers (this one
# is NOT deleted, since it's the actual data we care about keeping).
LOG_FILE_PATH = os.path.join(os.getcwd(), "vbat_capture_log.csv")


def read_channel_averages(csv_path, channels):
    """Read the combined analog.csv (one column per channel, plus a time
    column) and return {channel: average} across all rows."""
    sums = {ch: 0.0 for ch in channels}
    count = 0

    with open(csv_path, newline='') as f:
        reader = csv.reader(f)
        header = next(reader)  # e.g. ["Time [s]", "Channel 4", "Channel 5"]

        col_index = {}
        for ch in channels:
            for i, col_name in enumerate(header):
                if col_name.strip().lower() == f"channel {ch}":
                    col_index[ch] = i
                    break
            else:
                raise ValueError(f"Could not find column for channel {ch} in header: {header}")

        for row in reader:
            for ch in channels:
                sums[ch] += float(row[col_index[ch]])
            count += 1

    averages = {ch: (sums[ch] / count if count > 0 else float('nan')) for ch in channels}
    return averages, count


def ensure_log_header():
    if not os.path.exists(LOG_FILE_PATH):
        with open(LOG_FILE_PATH, 'w', newline='') as f:
            writer = csv.writer(f)
            header = ["timestamp", "elapsed_s", "last_sample", "vbat_history_back"]
            header += [f"ch{ch}_avg_V" for ch in ANALOG_CHANNELS]
            header += ["current_computed_mA", "capacity_used_mAh"]
            writer.writerow(header)


def run_capture_and_average(manager, device_configuration, capture_configuration):
    """Runs a single 0.5s capture, exports it, computes per-channel averages,
    cleans up the export, and returns {channel: average}."""
    with manager.start_capture(
        device_id=DEVICE_ID,
        device_configuration=device_configuration,
        capture_configuration=capture_configuration) as capture:

        capture.wait()

        os.makedirs(TEMP_EXPORT_DIR, exist_ok=True)
        capture.export_raw_data_csv(
            directory=TEMP_EXPORT_DIR,
            analog_channels=ANALOG_CHANNELS,
        )

        csv_path = os.path.join(TEMP_EXPORT_DIR, "analog.csv")
        if not os.path.exists(csv_path):
            print(f"Warning: expected export file not found: {csv_path}")
            channel_averages = {ch: float('nan') for ch in ANALOG_CHANNELS}
        else:
            channel_averages, _ = read_channel_averages(csv_path, ANALOG_CHANNELS)

    shutil.rmtree(TEMP_EXPORT_DIR, ignore_errors=True)
    return channel_averages


class LivePlot:
    """Maintains a 2x2 grid of subplots, updated as new samples arrive."""

    def __init__(self):
        plt.ion()
        self.fig, self.axs = plt.subplots(2, 2, figsize=(10, 8))
        self.fig.suptitle("LiPo Discharge Curve - Live")

        self.t = []
        self.ch4 = []
        self.ch5 = []
        self.vbat_history_back = []
        self.current_computed = []
        self.capacity_used = []

        self._configure_axes()
        self.fig.tight_layout(rect=[0, 0, 1, 0.96])
        plt.show(block=False)

    def _configure_axes(self):
        ax = self.axs[0, 0]
        ax.set_title("VBAT history vs Ch5")
        ax.set_xlabel("Ch5 (V)")
        ax.set_ylabel("vbat_history_back")

        ax = self.axs[0, 1]
        ax.set_title("Current vs Time")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Current (mA)")

        ax = self.axs[1, 0]
        ax.set_title("Capacity Used vs Time")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Capacity used (mAh)")

        ax = self.axs[1, 1]
        ax.set_title("Battery Voltage vs State of Charge")
        ax.set_xlabel("SoC (%)")
        ax.set_ylabel("Battery Voltage (mV)")
        ax.set_xlim(100, 0)

    def add_sample(self, t, ch4, ch5, vbat_history_back, current_computed, capacity_used):
        self.t.append(t)
        self.ch4.append(ch4)
        self.ch5.append(ch5)
        self.vbat_history_back.append(vbat_history_back)
        self.current_computed.append(current_computed)
        self.capacity_used.append(capacity_used)
        self._redraw()

    def _redraw(self):
        for ax_row in self.axs:
            for ax in ax_row:
                ax.clear()
        self._configure_axes()

        self.axs[0, 0].scatter(self.ch5, self.vbat_history_back, s=15, color='tab:blue')

        self.axs[0, 1].plot(self.t, self.current_computed, color='tab:orange')

        self.axs[1, 0].plot(self.t, self.capacity_used, color='tab:green')

        soc_percent = [(CAPACITY_TOTAL_MAH - c) / CAPACITY_TOTAL_MAH * 100.0 for c in self.capacity_used]
        ch4_mV = [v * 1000.0 for v in self.ch4]
        self.axs[1, 1].scatter(soc_percent, ch4_mV, s=15, color='tab:red')

        self.fig.canvas.draw()
        self.fig.canvas.flush_events()
        plt.pause(0.001)


def main():
    ensure_log_header()
    live_plot = LivePlot()

    start_time = time.monotonic()
    last_valid_message_time = time.monotonic()
    last_sample_time = None
    prev_current = 0.0
    capacity_used = 0.0

    with automation.Manager.connect(port=10430) as manager:

        device_configuration = automation.LogicDeviceConfiguration(
            enabled_analog_channels=ANALOG_CHANNELS,
            analog_sample_rate=SAMPLE_RATE,
        )

        capture_configuration = automation.CaptureConfiguration(
            capture_mode=automation.TimedCaptureMode(duration_seconds=CAPTURE_DURATION_S)
        )

        with serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout=SERIAL_READ_TIMEOUT_S) as ser:
            print(f"Listening on {SERIAL_PORT} at {SERIAL_BAUDRATE} baud...")

            while True:
                # --- Stop condition: no valid message for IDLE_TIMEOUT_S ---
                if time.monotonic() - last_valid_message_time > IDLE_TIMEOUT_S:
                    print(f"No valid message received for {IDLE_TIMEOUT_S}s. Stopping.")
                    break

                raw_line = ser.readline()
                if not raw_line:
                    continue  # readline timed out with no data, loop back to check idle timeout

                try:
                    line = raw_line.decode('utf-8', errors='replace').strip()
                except UnicodeDecodeError:
                    continue

                match = VBAT_LINE_RE.match(line)
                if not match:
                    continue  # not the message we're waiting for

                last_valid_message_time = time.monotonic()
                last_sample = float(match.group(1))
                vbat_history_back = float(match.group(2))
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

                print(f"[{timestamp}] Trigger received: "
                      f"last_sample={last_sample}, vbat_history_back={vbat_history_back}")

                channel_averages = run_capture_and_average(
                    manager, device_configuration, capture_configuration
                )
                ch4 = channel_averages.get(4, float('nan'))
                ch5 = channel_averages.get(5, float('nan'))

                # --- Compute current and integrate capacity used ---
                now = time.monotonic()
                elapsed_s = now - start_time
                current_computed = (ch5 - ch4 - OFFSET) / R * 1000.0  # mA

                if last_sample_time is not None:
                    dt_hours = (now - last_sample_time) / 3600.0
                    capacity_used += (abs(current_computed) + abs(prev_current)) / 2.0 * dt_hours
                last_sample_time = now
                prev_current = current_computed

                print(f"[{timestamp}] ch4={ch4:.6f} V, ch5={ch5:.6f} V, "
                      f"current={current_computed:.3f} mA, capacity_used={capacity_used:.3f} mAh")

                # --- Update live plots ---
                live_plot.add_sample(
                    elapsed_s, ch4, ch5, vbat_history_back, current_computed, capacity_used
                )

                # --- Append to persistent log ---
                with open(LOG_FILE_PATH, 'a', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow([
                        timestamp, elapsed_s, last_sample, vbat_history_back,
                        ch4, ch5, current_computed, capacity_used
                    ])

    plt.ioff()
    print("Done. Close the plot window to exit.")
    plt.show()


if __name__ == "__main__":
    main()
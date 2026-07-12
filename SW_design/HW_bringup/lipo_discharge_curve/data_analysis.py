import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import cumulative_trapezoid

BATTERY_CAPACITY_mAh = 500  # nominal pack capacity used for SoC%

# --- Helper: load + process a dataset ---
def load_and_process(csv_path, R_Shunt=1, V_offset=0.03):
    df = pd.read_csv(csv_path)
    df = df.sort_values("time_since_start(s)").reset_index(drop=True)

    time_vec = df["time_since_start(s)"].to_numpy()
    voltage_bat_alw_on_vec = df["VBAT_ALW_ON"].to_numpy()
    voltage_green_vec = df["V_GRN"].to_numpy()
    voltage_yellow_vec = df["V_YLW"].to_numpy()

    voltage_coorected_yellow_vec = voltage_yellow_vec - V_offset
    voltage_coorected_green_vec = voltage_green_vec - V_offset

    current_vec = (voltage_coorected_yellow_vec - voltage_coorected_green_vec) / R_Shunt

    consumed_capacity_As = cumulative_trapezoid(current_vec, time_vec, initial=0)
    consumed_capacity_mAh = consumed_capacity_As * (1000 / 3600)

    # SoC%: 100% at full charge, decreasing as the battery discharges
    soc_pct_vec = 100 - (consumed_capacity_mAh / BATTERY_CAPACITY_mAh) * 100

    return {
        "time_vec": time_vec,
        "voltage_bat_alw_on_vec": voltage_bat_alw_on_vec,
        "voltage_green_vec": voltage_green_vec,
        "voltage_yellow_vec": voltage_yellow_vec,
        "current_vec": current_vec,
        "consumed_capacity_mAh": consumed_capacity_mAh,
        "voltage_coorected_yellow_vec": voltage_coorected_yellow_vec,
        "soc_pct_vec": soc_pct_vec,
    }


# --- Helper: average two curves that live on different x-grids ---
def average_curve(x1, y1, x2, y2, num=200):
    order1 = np.argsort(x1)
    order2 = np.argsort(x2)
    x1s, y1s = x1[order1], y1[order1]
    x2s, y2s = x2[order2], y2[order2]

    x_lo = max(x1s.min(), x2s.min())
    x_hi = min(x1s.max(), x2s.max())
    if x_lo >= x_hi:
        return np.array([]), np.array([])

    x_common = np.linspace(x_lo, x_hi, num)
    y1_interp = np.interp(x_common, x1s, y1s)
    y2_interp = np.interp(x_common, x2s, y2s)
    y_avg = (y1_interp + y2_interp) / 2.0
    return x_common, y_avg


# --- Helper: plot one dataset onto the shared axes ---
def plot_dataset(ax1, ax2, ax3, ax4, data, color, label):
    time_vec = data["time_vec"]
    current_vec = data["current_vec"]
    consumed_capacity_mAh = data["consumed_capacity_mAh"]
    voltage_coorected_yellow_vec = data["voltage_coorected_yellow_vec"]
    voltage_bat_alw_on_vec = data["voltage_bat_alw_on_vec"]
    voltage_green_vec = data["voltage_green_vec"]
    soc_pct_vec = data["soc_pct_vec"]

    ax1.plot(time_vec / 60, current_vec * 1000, color=color, linewidth=1, label=label)
    ax2.plot(consumed_capacity_mAh, voltage_coorected_yellow_vec, color=color, linewidth=1, label=label)

    ax3.scatter(voltage_bat_alw_on_vec, voltage_green_vec, color=color, s=8, alpha=0.6, label=f"{label} data")
    slope, intercept = np.polyfit(voltage_bat_alw_on_vec, voltage_green_vec, 1)
    fit_x = np.linspace(voltage_bat_alw_on_vec.min(), voltage_bat_alw_on_vec.max(), 200)
    fit_y = slope * fit_x + intercept
    sign = "+" if intercept >= 0 else "-"
    formula_str = f"y = {slope:.4f}x {sign} {abs(intercept):.4f}"
    ax3.plot(fit_x, fit_y, color=color, linewidth=1.5, linestyle="--", label=f"{label} fit: {formula_str}")

    ax4.plot(soc_pct_vec-12.3, voltage_coorected_yellow_vec, color=color, linewidth=1, label=label)

    return slope, intercept


# --- 1. Load datasets ---
csv_path_1 = "./test1-wifi-radio-not-disabled/aggregate-raw-data.csv"
data1 = load_and_process(csv_path_1)

csv_path_2 = "./test2-wifi-disabled/aggregate-raw-data.csv"
data2 = load_and_process(csv_path_2)

# --- 2. Plot ---
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
ax1, ax2 = axes[0, 0], axes[0, 1]
ax3, ax4 = axes[1, 0], axes[1, 1]

slope1, intercept1 = plot_dataset(ax1, ax2, ax3, ax4, data1, color="tab:blue", label="WiFi Enabled")
slope2, intercept2 = plot_dataset(ax1, ax2, ax3, ax4, data2, color="tab:red", label="WiFi Disabled")

# --- Subplot 3: average of the two lines of best fit ---
slope_avg = (slope1 + slope2) / 2.0
intercept_avg = (intercept1 + intercept2) / 2.0
x_lo3 = min(data1["voltage_bat_alw_on_vec"].min(), data2["voltage_bat_alw_on_vec"].min())
x_hi3 = max(data1["voltage_bat_alw_on_vec"].max(), data2["voltage_bat_alw_on_vec"].max())
fit_x_avg = np.linspace(x_lo3, x_hi3, 200)
fit_y_avg = slope_avg * fit_x_avg + intercept_avg
sign_avg = "+" if intercept_avg >= 0 else "-"
formula_avg_str = f"y = {slope_avg:.4f}x {sign_avg} {abs(intercept_avg):.4f}"
ax3.plot(fit_x_avg, fit_y_avg, color="black", linewidth=1.5, linestyle="--",
          label=f"Average fit: {formula_avg_str}")

# --- Subplot 4: average discharge curve (SoC%) ---
x_avg4, y_avg4 = average_curve(
    data1["soc_pct_vec"], data1["voltage_coorected_yellow_vec"],
    data2["soc_pct_vec"], data2["voltage_coorected_yellow_vec"]
)
ax4.plot(x_avg4-12.3, y_avg4, color="black", linewidth=1.5, linestyle="--", label="Average")

# --- Labels/titles ---
ax1.set_xlabel("Time since start (min)")
ax1.set_ylabel("Current (mA)")
ax1.set_title("Current vs Time")
ax1.set_ylim([0, 140])
ax1.grid(True, alpha=0.3)
ax1.legend(loc="best", fontsize=8)

ax2.set_xlabel("Consumed Capacity (mAh)")
ax2.set_ylabel("Voltage Corrected Yellow (V)")
ax2.set_title("Discharge Curve: Voltage vs Consumed Capacity")
ax2.grid(True, alpha=0.3)
ax2.legend(loc="best", fontsize=8)

ax3.set_xlabel("ESP32 ADC Reported Voltage (V)")
ax3.set_ylabel("Saleae Logic 8 Reported Voltage (bias-corrected) (V)")
ax3.set_title("ESP32 ADC Calibration Curve")
ax3.grid(True, alpha=0.3)
ax3.legend(loc="best", fontsize=7)

ax4.set_xlim(100, 0)
ax4.set_xlabel("SoC % (100% = full charge)")
ax4.set_ylabel("Voltage Corrected Yellow (V)")
ax4.set_title("Discharge Curve: Voltage vs SoC%")
ax4.grid(True, alpha=0.3)
ax4.legend(loc="best", fontsize=8)

fig.tight_layout()
plt.show()
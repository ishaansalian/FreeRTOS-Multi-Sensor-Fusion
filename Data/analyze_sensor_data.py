import pandas as pd
import matplotlib.pyplot as plt

# File names
files = {
    "IMU": "imu.csv",
    "ENV": "env.csv",
    "LIGHT": "light.csv"
}

# Load data
dfs = {}
for key, path in files.items():
    dfs[key] = pd.read_csv(path)
    print(f"{key}: {len(dfs[key])} samples loaded")

# Compute inter-sample times
for key, df in dfs.items():
    df["teensy_dt"] = df["teensy_ms"].diff()
    df["esp_dt"] = df["esp_ms"].diff()

# Plot inter-sample times separately
for key, df in dfs.items():
    plt.figure(figsize=(8, 4))
    plt.plot(df["teensy_dt"], label="ΔTeensy (ms)")
    plt.plot(df["esp_dt"], label="ΔESP (ms)")
    plt.title(f"{key} — Inter-sample Time")
    plt.xlabel("Sample index")
    plt.ylabel("Δ time (ms)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

# Plot all inter-sample times together
plt.figure(figsize=(8, 4))
for key, df in dfs.items():
    plt.plot(df["teensy_dt"], label=f"{key} ΔTeensy")
plt.title("All Sensors — Teensy Inter-sample Time")
plt.xlabel("Sample index")
plt.ylabel("Δ time (ms)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# Multi-subplot of sensor values
fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=False)
for i, (key, df) in enumerate(dfs.items()):
    col = [c for c in df.columns if c not in ["teensy_ms", "esp_ms", "sensor", "teensy_dt", "esp_dt"]]
    axes[i].plot(df["teensy_ms"], df[col[0]], label=col[0])
    axes[i].set_title(f"{key} — {col[0]} vs Time")
    axes[i].set_ylabel(col[0])
    axes[i].grid(True)
plt.tight_layout()
plt.show()

# All sensors on same plane (first numeric column)
plt.figure(figsize=(10, 5))
for key, df in dfs.items():
    col = [c for c in df.columns if c not in ["teensy_ms", "esp_ms", "sensor", "teensy_dt", "esp_dt"]]
    plt.plot(df["teensy_ms"], df[col[0]], label=f"{key} ({col[0]})")
plt.title("All Sensors — First Column Overlay")
plt.xlabel("Teensy time (ms)")
plt.ylabel("Sensor value")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# ESP vs Teensy time alignment (drift)
plt.figure(figsize=(8, 4))
for key, df in dfs.items():
    plt.plot(df["teensy_ms"], df["esp_ms"], label=f"{key}")
plt.title("ESP vs Teensy Time Alignment")
plt.xlabel("Teensy time (ms)")
plt.ylabel("ESP time (ms)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

print("\n✅ Analysis complete. Check for gaps, jitter, or drift in the plots above.")
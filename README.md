A real-time multi-sensor data acquisition system using ESP32 and FreeRTOS.

## Overview

This project demonstrates embedded systems fundamentals through hands-on implementation of task scheduling, resource synchronization, and interrupt handling. Three sensors operating at different rates share a single I2C bus while logging data via UART to a Teensy for SD card storage.

**Hardware:**
- ESP32 (FreeRTOS dual-core)
- MPU6050 IMU (100Hz accelerometer/gyroscope)
- TSL2591 Light Sensor (0.5Hz ambient light)
- AHT20 Environmental Sensor (1Hz temperature/humidity)
- Teensy 3.6 (for SD card capability)

## Architecture

**Five concurrent FreeRTOS tasks:**
1. `TaskIMU` (Priority 3, 100Hz) - High-speed motion sensing
2. `TaskEnvironmental` (Priority 2, 1Hz) - Temperature/humidity monitoring
3. `TaskLight` (Priority 1, 0.5Hz) - Ambient light detection
4. `TaskIMUAlert` (Priority 5) - Interrupt-driven emergency detection, later deleted
5. `TaskLightAlert` (Priority 4) - Interrupt-driven brightness alerts, later deleted

**Synchronization:**
- `i2cMutex` - Protects shared I2C bus
- `serialMutex` - Prevents garbled UART output
- CSV-formatted logging for post-analysis

## My Shortcomings

- Failed to maintain stable interrupt operation with all three sensors active
- Did not implement DMA for I2C to eliminate blocking delays
- Clock synchronization between ESP32 and Teensy was not addressed
- Task priorities and mutex hold times were not optimally balanced, resulting in significant timing jitter
- Interrupt clear sequences were too complex for reliable operation under mutex contention

# 📐🛩️ UAV Dual IMU Evaluation Board

![Embedded](https://img.shields.io/badge/Embedded-Teensy-blue)
![C++](https://img.shields.io/badge/C++-Firmware-green)
![Sensors](https://img.shields.io/badge/Sensors-IMU%20Fusion-orange)
![PCB](https://img.shields.io/badge/PCB-KiCad-lightgrey)
![License](https://img.shields.io/badge/License-MIT-lightgrey)

## Overview

This repository documents an **updated inertial sensing stage for an unmanned aerial vehicle (UAV)**, developed as a **standalone PCB** to improve measurement quality, stability, and robustness of inertial measurements.

Originally, the UAV used an **MPU6050 IMU** integrated into the control PCB for inertial measurements.  
In this revision, inertial sensing was **fully separated into a dedicated evaluation board**, enabling **simultaneous measurement and comparison** of two different IMUs:

- **BMI088** (Bosch)
- **ICM-42605** (TDK InvenSense)

This modular architecture supports informed hardware decisions before final integration into the flight control system.

## 📂 Contents

- `/Hardware` → schematic, Gerbers and BOM+POS for PCBA.
- `/test_dual_angle_failover` → C code for Teensy/Arduino.

## 🌐 YouTube

📺 [Failover IMU](https://youtube.com/shorts/ujR3OjFHcZo)

## 🔗 Project Context

This dual IMU evaluation board is part of the ongoing hardware updates for the **DIY UAV project**.

The main project repository documents the overall UAV system, including mechanical design, control architecture, and system integration:

- **DIY UAV – Main Project Repository**  
  [DIY UAV](https://github.com/CrissCCL/DIY_UAV)

This repository focuses exclusively on **inertial sensor evaluation and validation**, separated from the control and power stages to improve system robustness and scalability.

## 🧠 Inertial Sensing Stage Description

The board integrates two IMUs operating simultaneously on the same PCB.

### Integrated Sensors

- **BMI088** (Bosch)
  - Industrial-grade accelerometer and gyroscope
  - High vibration tolerance
  - Designed for harsh motion environments

- **ICM-42605** (TDK InvenSense)
  - High-performance accelerometer and gyroscope
  - Low noise density
  - High bandwidth suitable for dynamic maneuvers

Both sensors are powered, clocked, and sampled under the same conditions, allowing direct and fair comparison.

## 🔗 Measurement Architecture

Both IMUs are integrated on the same PCB and operate simultaneously under identical conditions.

```text
IMU Evaluation Board
  │
  ├── BMI088     ──► Accel + Gyro Data
  │
  └── ICM-42605  ──► Accel + Gyro Data
            │
            └──► MCU / Data Acquisition
```

## 🎯 Evaluation Objectives

The purpose of this board is to:

- Improve **measurement quality and stability**
- Evaluate noise characteristics under identical conditions
- Compare dynamic response of different IMUs
- Enable **simultaneous inertial measurements**
- Support future **sensor fusion and redundancy strategies**

These objectives are critical for robust state estimation in UAV applications.

## 🔄 Measurement Strategy

The evaluation methodology ensures consistency across both sensors:

- Same mechanical mounting
- Same PCB layout and ground reference
- Same power supply domain
- Same sampling rate and acquisition timing
- Same data processing pipeline

This guarantees that observed differences are attributable to **sensor characteristics**, not system-level artifacts.

## ✅ Test Results — IMU Failover (IMU1 → IMU2)

A failover test was performed to validate the system behavior when the primary IMU becomes unavailable or unstable.

### Test Objective
Verify that the system can automatically switch from **IMU1** to **IMU2** during runtime, maintaining valid inertial measurements.

### Test Method
- **IMU1** was configured as the primary sensor at startup.
- A fault condition was introduced on IMU1 (disconnect / communication loss / invalid readings).
- The system monitored the IMU health status and triggered a failover event.
- After failover, **IMU2** was selected as the active sensor.

### Failover Criteria (Health Check)
The system triggers the IMU switch when one of the following conditions is detected:
- I2C timeout / no response
- Out-of-range or invalid sensor values
- Persistent mismatch between IMU1 and IMU2
- Consecutive mismatch counter exceeding a threshold

### 🔁 Dual-IMU Comparison + Automatic Failover Logic (Flowchart)

<p align="center">
  <img alt="flowchart" src="https://github.com/user-attachments/assets/ee94ecae-a29f-4c99-bcbf-9195b1177c3d" width="400">
</p>

### Results Summary
- ✅ Failover successfully triggered when IMU1 fault condition was detected
- ✅ IMU2 became the active sensor and continued streaming valid data
- ✅ No system reset was required during the switch
- ✅ Recovery behavior confirmed (optional: switch back to IMU1 if restored)

### Evidence (Plots / Logs)

**Example output format (CSV):**
`roll, pitch, yaw, primary, mismatch, cnt`

- `primary = 0` → IMU1 active  
- `primary = 1` → IMU2 active  


<p align="center">
<img alt="imu_failover_test" src="https://github.com/user-attachments/assets/10939579-b64f-4e18-8dea-3ba132484fbc" width="700">
</p>

> 📌 **Note:** During the failover event, `primary` changes from **0 → 1**, confirming the sensor switch.

## 🖼️ PCB Render Visualization

<table>
  <tr>
    <td align="center">
      <img alt="dual IMU_module top V2" src="https://github.com/user-attachments/assets/2a4c3bcc-786a-4df1-a85d-c723fea58126" width="550"><br>
      <sub> Dual IMU PCB V2 – Top View </sub>
    </td>
    <td align="center">
        <img  alt="dual IMU_module botton V2" src="https://github.com/user-attachments/assets/68846fde-b554-45c8-acc2-ae7cef270093" width="550"><br>
      <sub> Dual IMU PCB V2 – Bottom View </sub>
    </td>
  </tr>
</table>

## ⚡ Physical Prototype

The following image shows the complete setup of the **prototype version V1**:

<table>
  <tr>
    <td align="center">
      <img alt="dual IMU_module PCB" V1 src="https://github.com/user-attachments/assets/3224257f-2846-4299-a638-78842e59a997" width="550"><br>
      <sub> Dual IMU PCB unmounted </sub>
    </td>
    <td align="center">
        <img  alt="dual IMU_module PCB V1 " src="https://github.com/user-attachments/assets/5d49c68f-5d1c-4397-876b-87cf7257769c" width="550"><br>
      <sub> Dual IMU PCB </sub>
    </td>
  </tr>
</table>

## ⚠️ Disclaimer

This project is intended **for educational and experimental purposes only**.  
It is not a flight-certified inertial sensing module.

## 🤝 Support projects
 Support me on Patreon [https://www.patreon.com/c/CrissCCL](https://www.patreon.com/c/CrissCCL)

## 📜 License
MIT License

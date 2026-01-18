# 🧠🛩️ UAV Dual IMU Evaluation Board

## Overview

This repository documents an **updated inertial sensing stage for an unmanned aerial vehicle (UAV)**, developed as a **standalone PCB** to improve measurement quality, stability, and robustness of inertial measurements.

Originally, the UAV used an **MPU6050 IMU** integrated into the control PCB for inertial measurements.  
In this revision, inertial sensing was **fully separated into a dedicated evaluation board**, enabling **simultaneous measurement and comparison** of two different IMUs:

- **BMI088** (Bosch)
- **ICM-42605** (TDK InvenSense)

This modular architecture supports informed hardware decisions before final integration into the flight control system.

## 📂 Contents

- `/Hardware` → schematic, Gerbers and BOM+POS for PCBA.

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

## 🖼️ PCB Render Visualization

<table>
  <tr>
    <td align="center">
      <img alt="dual IMU_module top V2" src="https://github.com/user-attachments/assets/1fffd14f-142e-477e-a7c2-cd1da5cd41ec" width="550"><br>
      <sub> Dual IMU PCB V2 – Top View </sub>
    </td>
    <td align="center">
        <img  alt="dual IMU_module botton V2" src="https://github.com/user-attachments/assets/3fea04d8-1986-430d-943e-597cff942891" width="550"><br>
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

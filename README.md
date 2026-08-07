# Rocket Flight Computer

A custom STM32F405-based rocket flight computer designed for high-power rocketry.

![Status](https://img.shields.io/badge/Status-In%20Development-blue)
![Language](https://img.shields.io/badge/C-STM32-success)
![MCU](https://img.shields.io/badge/MCU-STM32F405-orange)

---

## Overview

This project implements a complete embedded avionics system for high-power rockets.

Features include:

- BMP581 Barometer
- LSM6DSO32 IMU
- GPS
- SD Card Logging
- USB Serial Interface
- Flight State Detection
- Kalman Filter for Altitude and Velocity Estimation

---

## Hardware

### MCU

STM32F405

### Sensors

- BMP581
- LSM6DSO32
- GPS Receiver

### Interfaces

- I2C
- UART
- USB CDC
- SDIO

---

## Software Architecture

Firmware is written entirely in C using STM32CubeIDE.

Modules include:

- Sensor Drivers
- Flight Computer Core
- Data Logger
- USB Terminal
- GPS Parser
- Kalman Filter

---

## Current Status

- Hardware Complete
- Firmware Complete
- Ground Testing Complete
- Flight Testing In Progress

---

## Future Improvements

- Dual Deployment
- Radio Telemetry
- Flash Memory Backup
- Advanced State Estimation

---

## Author

Konstantinos Stamatakos
Tufts University

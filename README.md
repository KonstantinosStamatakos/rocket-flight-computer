# Rocket Flight Computer

Custom STM32F405-based flight computer designed for onboard data acquisition and state estimation in a high-power rocket.

The board integrates a barometer, IMU, GPS receiver, microSD storage, and USB interface on a custom PCB. Embedded firmware handles sensor acquisition, calibration, data logging, and real-time estimation of altitude and vertical velocity.

The system has completed ground testing and is being prepared for its first flight test.

---

## Overview

The goal of this project was to build the complete avionics system rather than use a commercial flight computer or separate development boards.

The project includes both the custom hardware and the embedded firmware required to:

- acquire barometer, IMU, and GPS data
- estimate altitude and vertical velocity in real time
- log flight data to an onboard microSD card
- provide USB serial output for testing and debugging

---

## Hardware

The flight computer is built around an **STM32F405** microcontroller.

### Sensors and Interfaces

| Component | Purpose | Interface |
|---|---|---|
| STM32F405 | Main processor | — |
| BMP581 | Barometric pressure / altitude | I²C |
| LSM6DSO32 | Accelerometer and gyroscope | I²C |
| GPS receiver | Position and navigation data | UART |
| microSD | Onboard data logging | SDIO |
| USB | Debugging and serial output | USB CDC |

The hardware was designed as a custom PCB for integration into the rocket's avionics bay.

---

## Firmware

The firmware is written in **C** using the STM32 development environment.

Each major subsystem is separated into its own module:

```text
bmp581.c / bmp581.h
imu.c / imu.h
gps.c / gps.h
sd_logger.c / sd_logger.h
vertical_filter.c / vertical_filter.h
```

The main application coordinates sensor sampling, state estimation, data logging, and USB output.

---

## State Estimation

Barometric altitude provides a useful absolute altitude measurement but contains measurement noise. Accelerometer data responds quickly to changes in motion but accumulates error when integrated over time.

The flight computer combines these measurements using a **linear Kalman filter**.

The filter estimates two vertical states:

```text
altitude
vertical velocity
```

Acceleration is used to predict the vehicle's motion, while barometric altitude is used to correct the estimate.

This provides a continuous estimate of the rocket's vertical motion without relying on either sensor independently.

---

## Data Logging

Sensor and estimated-state data are recorded to an onboard microSD card in CSV format.

The logging system uses the STM32 **SDIO interface** with **FatFs** and records data for post-flight analysis.

Logged data includes measurements from the onboard sensors together with the output of the vertical-state estimator.

Example ground-test logs are included in the repository.

---

## Testing

The system has been tested on the ground with all major subsystems operating together.

Testing included:

- barometer and IMU communication
- GPS data reception
- sensor calibration
- USB CDC output
- microSD initialization and file creation
- continuous CSV logging
- altitude and vertical-velocity estimation

A significant part of the project involved hardware/firmware debugging, particularly during microSD and sensor bring-up.

---

## Current Status

The flight computer is currently operational in ground testing.

Sensor acquisition, GPS communication, state estimation, USB debugging, and microSD logging have been implemented and tested.

The next major step is flight testing the system onboard the Level 2 rocket.

---

## Tools

**Hardware:** STM32F405, BMP581, LSM6DSO32, GPS, microSD  
**Firmware:** C, STM32 HAL, FatFs  
**Interfaces:** I²C, UART, SDIO, USB CDC  
**Analysis:** Python

---


## Next Steps

- Complete integrated pre-flight testing
- Perform first flight test
- Analyze recorded flight data
- Compare barometric measurements with the estimated altitude and vertical velocity
- Use flight data to tune the estimator for future launches

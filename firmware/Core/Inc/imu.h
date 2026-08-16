/*
 * imu.h
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */

#ifndef IMU_H
#define IMU_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/*
 * ============================================================
 * imu.h
 * ------------------------------------------------------------
 * Minimal LSM6DSO32 IMU driver interface.
 *
 * We expose:
 * - init
 * - read all sensor values
 *
 * For now, we store:
 * - temperature
 * - raw accel/gyro
 * - converted accel in g
 * - converted gyro in deg/s
 * ============================================================
 */

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
} LSM6DSO32_t;

typedef struct
{
    float temp_c;

    int16_t acc_raw_x;
    int16_t acc_raw_y;
    int16_t acc_raw_z;

    int16_t gyro_raw_x;
    int16_t gyro_raw_y;
    int16_t gyro_raw_z;

    float acc_x_g;
    float acc_y_g;
    float acc_z_g;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
} LSM6DSO32_Data_t;

uint8_t LSM6DSO32_Init(LSM6DSO32_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);
uint8_t LSM6DSO32_ReadAll(LSM6DSO32_t *dev, LSM6DSO32_Data_t *data);

#endif

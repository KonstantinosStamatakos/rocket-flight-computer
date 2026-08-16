/*
 * imu.c
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */


#include "imu.h"

/*
 * ============================================================
 * imu.c
 * ------------------------------------------------------------
 * Minimal LSM6DSO32 driver.
 *
 * This version configures:
 * - accelerometer at 104 Hz, ±8 g
 * - gyroscope at 104 Hz, ±250 dps
 *
 * Why ±8 g for now:
 * - more headroom than ±4 g
 * - still decent resolution
 *
 * Later for flight we may increase range again depending on
 * expected boost acceleration.
 * ============================================================
 */

#define LSM6DSO32_REG_WHO_AM_I   0x0F
#define LSM6DSO32_REG_CTRL1_XL   0x10
#define LSM6DSO32_REG_CTRL2_G    0x11
#define LSM6DSO32_REG_CTRL3_C    0x12
#define LSM6DSO32_REG_OUT_TEMP_L 0x20

#define LSM6DSO32_WHO_AM_I_VAL   0x6C

/*
 * CTRL1_XL = 0x48
 * - ODR_XL = 104 Hz
 * - FS_XL  = ±8 g
 *
 * CTRL2_G = 0x40
 * - ODR_G = 104 Hz
 * - FS_G  = ±250 dps
 *
 * CTRL3_C = 0x44
 * - BDU = 1 (block data update)
 * - IF_INC = 1 (auto-increment register address)
 */

/* Conversion constants for the selected ranges */
#define LSM6DSO32_ACC_SENSITIVITY_MG_LSB     0.244f
#define LSM6DSO32_GYRO_SENSITIVITY_MDPS_LSB  8.75f

static HAL_StatusTypeDef LSM6DSO32_WriteReg(LSM6DSO32_t *dev, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->hi2c,
                             dev->i2c_addr,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             HAL_MAX_DELAY);
}

static HAL_StatusTypeDef LSM6DSO32_ReadReg(LSM6DSO32_t *dev, uint8_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(dev->hi2c,
                            dev->i2c_addr,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            value,
                            1,
                            HAL_MAX_DELAY);
}

static HAL_StatusTypeDef LSM6DSO32_ReadBurst(LSM6DSO32_t *dev, uint8_t startReg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c,
                            dev->i2c_addr,
                            startReg,
                            I2C_MEMADD_SIZE_8BIT,
                            buf,
                            len,
                            HAL_MAX_DELAY);
}

uint8_t LSM6DSO32_Init(LSM6DSO32_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    uint8_t who_am_i = 0;

    dev->hi2c = hi2c;
    dev->i2c_addr = addr;

    if (LSM6DSO32_ReadReg(dev, LSM6DSO32_REG_WHO_AM_I, &who_am_i) != HAL_OK)
        return 0;

    if (who_am_i != LSM6DSO32_WHO_AM_I_VAL)
        return 0;

    if (LSM6DSO32_WriteReg(dev, LSM6DSO32_REG_CTRL3_C, 0x44) != HAL_OK)
        return 0;

    if (LSM6DSO32_WriteReg(dev, LSM6DSO32_REG_CTRL1_XL, 0x48) != HAL_OK)
        return 0;

    if (LSM6DSO32_WriteReg(dev, LSM6DSO32_REG_CTRL2_G, 0x40) != HAL_OK)
        return 0;

    HAL_Delay(20);
    return 1;
}

uint8_t LSM6DSO32_ReadAll(LSM6DSO32_t *dev, LSM6DSO32_Data_t *data)
{
    uint8_t buf[14];

    if (LSM6DSO32_ReadBurst(dev, LSM6DSO32_REG_OUT_TEMP_L, buf, 14) != HAL_OK)
        return 0;

    int16_t temp_raw   = (int16_t)((buf[1]  << 8) | buf[0]);
    int16_t gyro_x_raw = (int16_t)((buf[3]  << 8) | buf[2]);
    int16_t gyro_y_raw = (int16_t)((buf[5]  << 8) | buf[4]);
    int16_t gyro_z_raw = (int16_t)((buf[7]  << 8) | buf[6]);
    int16_t acc_x_raw  = (int16_t)((buf[9]  << 8) | buf[8]);
    int16_t acc_y_raw  = (int16_t)((buf[11] << 8) | buf[10]);
    int16_t acc_z_raw  = (int16_t)((buf[13] << 8) | buf[12]);

    data->temp_c = 25.0f + ((float)temp_raw / 256.0f);

    data->gyro_raw_x = gyro_x_raw;
    data->gyro_raw_y = gyro_y_raw;
    data->gyro_raw_z = gyro_z_raw;

    data->acc_raw_x = acc_x_raw;
    data->acc_raw_y = acc_y_raw;
    data->acc_raw_z = acc_z_raw;

    data->gyro_x_dps = ((float)gyro_x_raw * LSM6DSO32_GYRO_SENSITIVITY_MDPS_LSB) / 1000.0f;
    data->gyro_y_dps = ((float)gyro_y_raw * LSM6DSO32_GYRO_SENSITIVITY_MDPS_LSB) / 1000.0f;
    data->gyro_z_dps = ((float)gyro_z_raw * LSM6DSO32_GYRO_SENSITIVITY_MDPS_LSB) / 1000.0f;

    data->acc_x_g = ((float)acc_x_raw * LSM6DSO32_ACC_SENSITIVITY_MG_LSB) / 1000.0f;
    data->acc_y_g = ((float)acc_y_raw * LSM6DSO32_ACC_SENSITIVITY_MG_LSB) / 1000.0f;
    data->acc_z_g = ((float)acc_z_raw * LSM6DSO32_ACC_SENSITIVITY_MG_LSB) / 1000.0f;

    return 1;
}

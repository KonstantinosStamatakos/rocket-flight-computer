/*
 * bmp581.c
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */


#include "bmp581.h"

/*
 * ============================================================
 * bmp581.c
 * ------------------------------------------------------------
 * Minimal BMP581 driver.
 *
 * We do not implement every feature of the sensor.
 * We only configure enough to:
 * - confirm chip ID
 * - enable measurements
 * - read temperature and pressure
 * ============================================================
 */

/* Register map pieces we need */
#define BMP581_REG_CHIP_ID     0x01
#define BMP581_REG_TEMP_XLSB   0x1D
#define BMP581_REG_OSR_CONFIG  0x36
#define BMP581_REG_ODR_CONFIG  0x37
#define BMP581_REG_CMD         0x7E

#define BMP581_CHIP_ID_VALUE   0x50
#define BMP581_SOFT_RESET      0xB6

static HAL_StatusTypeDef BMP581_WriteReg(BMP581_t *dev, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->hi2c,
                             dev->i2c_addr,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             HAL_MAX_DELAY);
}

static HAL_StatusTypeDef BMP581_ReadReg(BMP581_t *dev, uint8_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(dev->hi2c,
                            dev->i2c_addr,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            value,
                            1,
                            HAL_MAX_DELAY);
}

static HAL_StatusTypeDef BMP581_ReadBurst(BMP581_t *dev, uint8_t startReg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c,
                            dev->i2c_addr,
                            startReg,
                            I2C_MEMADD_SIZE_8BIT,
                            buf,
                            len,
                            HAL_MAX_DELAY);
}

uint8_t BMP581_Init(BMP581_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    uint8_t chip_id = 0;

    dev->hi2c = hi2c;
    dev->i2c_addr = addr;

    /* Soft reset helps start from a known state */
    if (BMP581_WriteReg(dev, BMP581_REG_CMD, BMP581_SOFT_RESET) != HAL_OK)
        return 0;

    HAL_Delay(10);

    /* Verify chip identity */
    if (BMP581_ReadReg(dev, BMP581_REG_CHIP_ID, &chip_id) != HAL_OK)
        return 0;

    if (chip_id != BMP581_CHIP_ID_VALUE)
        return 0;

    /*
     * Example configuration:
     * - oversampling set
     * - normal mode enabled
     *
     * This is enough for a working first project.
     */
    if (BMP581_WriteReg(dev, BMP581_REG_OSR_CONFIG, 0x60) != HAL_OK)
        return 0;

    if (BMP581_WriteReg(dev, BMP581_REG_ODR_CONFIG, 0xDD) != HAL_OK)
        return 0;

    HAL_Delay(20);
    return 1;
}

uint8_t BMP581_ReadTempPressure(BMP581_t *dev, float *temperature_c, float *pressure_pa)
{
    uint8_t data[6];

    /*
     * Read:
     *   temp_xlsb, temp_lsb, temp_msb,
     *   press_xlsb, press_lsb, press_msb
     */
    if (BMP581_ReadBurst(dev, BMP581_REG_TEMP_XLSB, data, 6) != HAL_OK)
        return 0;

    uint32_t raw_temp  = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
    uint32_t raw_press = ((uint32_t)data[5] << 16) | ((uint32_t)data[4] << 8) | data[3];

    /*
     * Bosch conversion:
     *   temp = raw / 2^16
     *   press = raw / 2^6
     */
    *temperature_c = (float)raw_temp / 65536.0f;
    *pressure_pa   = (float)raw_press / 64.0f;

    return 1;
}

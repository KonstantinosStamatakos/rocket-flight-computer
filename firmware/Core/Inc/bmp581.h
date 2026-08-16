/*
 * bmp581.h
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */

#ifndef BMP581_H
#define BMP581_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/*
 * ============================================================
 * bmp581.h
 * ------------------------------------------------------------
 * Minimal BMP581 driver interface.
 *
 * We only expose:
 * - init
 * - read temperature + pressure
 *
 * That is enough for the first flight project stage.
 * ============================================================
 */

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
} BMP581_t;

uint8_t BMP581_Init(BMP581_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);
uint8_t BMP581_ReadTempPressure(BMP581_t *dev, float *temperature_c, float *pressure_pa);

#endif

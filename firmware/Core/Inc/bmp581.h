#ifndef BMP581_H
#define BMP581_H

#include "stm32f4xx_hal.h"
#include <stdint.h>


typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
} BMP581_t;

uint8_t BMP581_Init(BMP581_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);
uint8_t BMP581_ReadTempPressure(BMP581_t *dev, float *temperature_c, float *pressure_pa);

#endif

#ifndef SENSORS_H
#define SENSORS_H

#include "stm32f4xx_hal.h"
#include "imu.h"
#include "gps.h"
#include <stdint.h>


typedef struct
{
    /*
     * ---------------- BMP581 ----------------
     */

    uint8_t bmp_ok;

    float bmp_temp_c;
    float pressure_pa;
    float pressure_hpa;
    float baro_altitude_m;

    /*
     * Timestamp of barometer sample.
     */
    uint32_t baro_timestamp_ms;

    uint8_t baro_new_data;

    /*
     * ---------------- IMU ----------------
     */

    uint8_t imu_ok;
    LSM6DSO32_Data_t imu;

    uint32_t imu_timestamp_ms;
    uint8_t imu_new_data;

    /*
     * ---------------- GPS ----------------
     */

    uint8_t gps_ok;
    GPS_t gps;

    uint32_t gps_timestamp_ms;
    uint8_t gps_new_data;

} SensorsData_t;

uint8_t Sensors_Init(I2C_HandleTypeDef *hi2c,
                     UART_HandleTypeDef *huart);

uint8_t Sensors_UpdateImu(void);
uint8_t Sensors_UpdateBaro(void);

void Sensors_GpsRxComplete(UART_HandleTypeDef *huart);

SensorsData_t *Sensors_GetData(void);

#endif

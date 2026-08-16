/*
 * sensors.h
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "stm32f4xx_hal.h"
#include "imu.h"
#include "gps.h"
#include <stdint.h>

/*
 * ============================================================
 * COMBINED SENSOR DATA
 * ============================================================
 *
 * Αυτή η δομή περιέχει το τελευταίο έγκυρο sample από όλους
 * τους αισθητήρες.
 */

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
     * Timestamp του τελευταίου barometer sample.
     */
    uint32_t baro_timestamp_ms;

    /*
     * Γίνεται 1 μόνο όταν έχουμε καινούριο barometer sample.
     * Το FlightApp το μηδενίζει αφού χρησιμοποιήσει το sample.
     */
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

/*
 * Αρχικοποίηση όλων των sensor drivers.
 */
uint8_t Sensors_Init(I2C_HandleTypeDef *hi2c,
                     UART_HandleTypeDef *huart);

/*
 * Ξεχωριστά updates για κάθε sensor.
 *
 * Έτσι το FlightApp μπορεί να έχει διαφορετικό update rate
 * για IMU και barometer.
 */
uint8_t Sensors_UpdateImu(void);
uint8_t Sensors_UpdateBaro(void);

/*
 * Callback για GPS UART interrupt.
 *
 * Καλείται από το HAL_UART_RxCpltCallback().
 */
void Sensors_GpsRxComplete(UART_HandleTypeDef *huart);

/*
 * Επιστρέφει pointer στη δομή με τα τελευταία δεδομένα.
 */
SensorsData_t *Sensors_GetData(void);

#endif

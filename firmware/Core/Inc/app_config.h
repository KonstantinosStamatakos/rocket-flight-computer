#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/*
 * ============================================================
 * SENSOR I2C ADDRESSES
 * ============================================================
 */

#define BMP581_I2C_ADDR        (0x46U << 1)
#define LSM6DSO32_I2C_ADDR     (0x6AU << 1)

/*
 * ============================================================
 * APPLICATION UPDATE RATES
 * ============================================================
 */

#define IMU_PERIOD_MS          5U
#define BARO_PERIOD_MS         20U
#define FILTER_PERIOD_MS       5U
#define DEBUG_PERIOD_MS        500U
#define SD_LOG_PERIOD_MS       20U
#define SD_SYNC_PERIOD_MS      1000U
/*
 * Pressure low-pass filter.
 */
#define BARO_FILTER_ALPHA      0.10f


#define GRAVITY_MPS2           9.80665f
#define ACCEL_DEADBAND_MPS2    0.08f

#endif

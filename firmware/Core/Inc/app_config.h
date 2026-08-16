/*
 * app_config.h
 *
 *  Created on: Jun 9, 2026
 *      Author: deanstamatakos
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/*
 * ============================================================
 * SENSOR I2C ADDRESSES
 * ============================================================
 *
 * Το STM32 HAL περιμένει τη 7-bit I2C διεύθυνση μετατοπισμένη
 * κατά μία θέση προς τα αριστερά.
 */

#define BMP581_I2C_ADDR        (0x46U << 1)
#define LSM6DSO32_I2C_ADDR     (0x6AU << 1)

/*
 * ============================================================
 * APPLICATION UPDATE RATES
 * ============================================================
 *
 * IMU:
 * Διαβάζουμε acceleration και gyroscope κάθε 5 ms.
 * Αυτό αντιστοιχεί σε 200 Hz.
 *
 * Barometer:
 * Διαβάζουμε pressure/altitude κάθε 20 ms.
 * Αυτό αντιστοιχεί σε 50 Hz.
 *
 * Filter:
 * Το prediction χρησιμοποιεί κάθε νέο IMU sample.
 *
 * Debug:
 * Εκτυπώνουμε μόνο κάθε 500 ms, ώστε το USB printf να μην
 * επιβαρύνει το πραγματικό flight loop.
 */

#define IMU_PERIOD_MS          5U
#define BARO_PERIOD_MS         20U
#define FILTER_PERIOD_MS       5U
#define DEBUG_PERIOD_MS        500U
#define SD_LOG_PERIOD_MS       20U
#define SD_SYNC_PERIOD_MS      1000U
/*
 * Pressure low-pass filter.
 *
 * Μεγαλύτερο alpha:
 * - ταχύτερη αντίδραση
 * - περισσότερος θόρυβος
 *
 * Μικρότερο alpha:
 * - πιο ομαλό altitude
 * - μεγαλύτερη καθυστέρηση
 *
 * Το 0.10 είναι λογικό αρχικό σημείο για bench testing.
 */
#define BARO_FILTER_ALPHA      0.10f

/*
 * Gravity constant σε m/s².
 */
#define GRAVITY_MPS2           9.80665f

/*
 * Μικρό acceleration deadband.
 *
 * Acceleration μικρότερο από αυτό θεωρείται πιθανό sensor noise.
 */
#define ACCEL_DEADBAND_MPS2    0.08f

#endif

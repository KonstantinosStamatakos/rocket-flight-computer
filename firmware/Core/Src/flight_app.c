/*
 * flight_app.c
 *
 *  Created on: Jun 9, 2026
 *      Author: deanstamatakos
 */
#include "flight_app.h"
#include "app_config.h"
#include "sensors.h"
#include "vertical_filter.h"
#include "main.h"
#include "sd_logger.h"

#include <stdio.h>
#include <stdint.h>

/*
 * Τα handles δημιουργούνται από το CubeMX μέσα στο main.c.
 */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

/*
 * ============================================================
 * APPLICATION TIMERS
 * ============================================================
 */

static uint32_t last_imu_ms = 0U;
static uint32_t last_baro_ms = 0U;
static uint32_t last_filter_ms = 0U;
static uint32_t last_debug_ms = 0U;
static uint32_t last_sd_log_ms = 0U;
static uint32_t last_sd_sync_ms = 0U;

/*
 * ============================================================
 * VERTICAL ESTIMATOR
 * ============================================================
 */

static VerticalFilter_t vertical_filter;

/*
 * Πραγματική stationary τιμή του Z accelerometer.
 *
 * Δεν υποθέτουμε ότι είναι ακριβώς 1.000 g.
 */
static float accel_z_rest_g = 1.0f;

/*
 * ============================================================
 * ACCELEROMETER STARTUP CALIBRATION
 * ============================================================
 */

static void FlightApp_CalibrateAccelerometer(void)
{
    float sum_z_g = 0.0f;
    uint32_t valid_samples = 0U;

    printf("Keep the board still for accelerometer calibration...\r\n");

    /*
     * 200 samples × 5 ms = περίπου 1 δευτερόλεπτο.
     */
    for (uint32_t i = 0U; i < 200U; i++)
    {
        if (Sensors_UpdateImu())
        {
            SensorsData_t *s = Sensors_GetData();

            sum_z_g += s->imu.acc_z_g;
            valid_samples++;
        }

        HAL_Delay(5U);
    }

    if (valid_samples > 0U)
    {
        accel_z_rest_g =
            sum_z_g / (float)valid_samples;
    }
    else
    {
        /*
         * Fallback μόνο αν αποτύχει η calibration.
         */
        accel_z_rest_g = 1.0f;
    }

    printf("Accelerometer Z rest value = %.5f g\r\n",
           accel_z_rest_g);
}

/*
 * ============================================================
 * FLIGHT APPLICATION INITIALIZATION
 * ============================================================
 */

void FlightApp_Init(void)
{
    uint8_t sensors_ok;

    /*
     * Πρώτα αρχικοποιούμε τους αισθητήρες.
     *
     * Αυτή η λειτουργία περιέχει calibration delays, επομένως
     * τα application timers ξεκινούν αργότερα.
     */
    sensors_ok = Sensors_Init(&hi2c1, &huart2);

    printf("Sensor initialization status = %u\r\n",
           sensors_ok);

    /*
     * Μετράμε τη stationary τιμή του accelerometer.
     *
     * Η πλακέτα πρέπει να βρίσκεται στον ίδιο προσανατολισμό
     * που θα έχει μέσα στον πύραυλο.
     */
    FlightApp_CalibrateAccelerometer();

    /*
     * Αρχικοποίηση vertical state estimator.
     *
     * Initial altitude = 0 m
     * Initial velocity = 0 m/s
     *
     * accel_var και baro_var είναι αρχικές tuning τιμές.
     */
    VerticalFilter_Init(&vertical_filter,
                        0.0f,
                        0.0f,
                        25.0f,
                        4.0f);

    SDLoggerStatus_t sd_status = SDLogger_Init();

    if (sd_status == SD_LOGGER_OK)
    {
        printf("SD logger started\r\n");
        printf("Log file: %s\r\n", SDLogger_GetFilename());
    }
    else
    {
        /*
         * Δεν σταματάμε το flight application αν αποτύχει η SD.
         *
         * Οι αισθητήρες και το estimator συνεχίζουν να λειτουργούν.
         */
        printf("SD logger initialization failed: %u\r\n",
               (unsigned int)sd_status);
    }

    /*
     * Ξεκινάμε όλους τους timers μετά την ολοκλήρωση των
     * calibration procedures.
     */
    uint32_t now = HAL_GetTick();

    last_imu_ms = now;
    last_baro_ms = now;
    last_filter_ms = now;
    last_debug_ms = now;
    last_sd_log_ms = now;
    last_sd_sync_ms = now;

    printf("Flight application started\r\n");
}

/*
 * ============================================================
 * MAIN NON-BLOCKING APPLICATION FUNCTION
 * ============================================================
 *
 * Αυτή η συνάρτηση καλείται συνέχεια από το while(1).
 *
 * Δεν περιέχει HAL_Delay().
 *
 * Κάθε task εκτελείται όταν περάσει το δικό του χρονικό
 * διάστημα.
 */

void FlightApp_Run(void)
{
    uint32_t now = HAL_GetTick();

    SensorsData_t *s = Sensors_GetData();

    /*
     * ========================================================
     * IMU TASK — 200 Hz
     * ========================================================
     */

    if ((uint32_t)(now - last_imu_ms) >= IMU_PERIOD_MS)
    {
        /*
         * Προχωρούμε τον scheduler κατά ακριβώς μία περίοδο,
         * αντί να γράψουμε last_imu_ms = now.
         *
         * Αυτό μειώνει το timing drift.
         */
        last_imu_ms += IMU_PERIOD_MS;

        Sensors_UpdateImu();
    }

    /*
     * ========================================================
     * BAROMETER TASK — 50 Hz
     * ========================================================
     */

    if ((uint32_t)(now - last_baro_ms) >= BARO_PERIOD_MS)
    {
        last_baro_ms += BARO_PERIOD_MS;

        Sensors_UpdateBaro();
    }

    /*
     * ========================================================
     * VERTICAL FILTER TASK — 200 Hz
     * ========================================================
     */

    if ((uint32_t)(now - last_filter_ms) >= FILTER_PERIOD_MS)
    {
        uint32_t elapsed_ms =
            (uint32_t)(now - last_filter_ms);

        /*
         * Χρησιμοποιούμε πραγματικό elapsed time.
         */
        float dt_s = (float)elapsed_ms * 0.001f;

        last_filter_ms = now;

        /*
         * Εκτελούμε prediction μόνο όταν υπάρχει πρόσφατο
         * και έγκυρο IMU sample.
         */
        if (s->imu_ok && dt_s > 0.0f && dt_s < 0.050f)
        {
            /*
             * Αφαιρούμε το measured resting acceleration.
             *
             * Παράδειγμα:
             *
             * measured rest = 1.017 g
             * current       = 1.020 g
             *
             * motion accel = 0.003 g
             */
            float accel_z_mps2 =
                (s->imu.acc_z_g - accel_z_rest_g)
                * GRAVITY_MPS2;

            /*
             * Μικρές τιμές πιθανότατα είναι sensor noise.
             */
            if (accel_z_mps2 > -ACCEL_DEADBAND_MPS2 &&
                accel_z_mps2 <  ACCEL_DEADBAND_MPS2)
            {
                accel_z_mps2 = 0.0f;
            }

            /*
             * Prediction από acceleration.
             */
            VerticalFilter_Predict(&vertical_filter,
                                   accel_z_mps2,
                                   dt_s);
        }

        /*
         * Barometer correction μόνο όταν υπάρχει νέο barometer
         * sample.
         *
         * Δεν θέλουμε να χρησιμοποιούμε το ίδιο barometer
         * measurement 4 φορές.
         */
        if (s->bmp_ok && s->baro_new_data)
        {
            VerticalFilter_UpdateBaro(
                &vertical_filter,
                s->baro_altitude_m);

            s->baro_new_data = 0U;
        }
    }

    /*
     * ============================================================
     * SD LOGGING TASK — 50 Hz
     * ============================================================
     *
     * Κάθε 20 ms δημιουργούμε ένα snapshot όλων των τελευταίων
     * δεδομένων και το γράφουμε στο CSV.
     */

    if ((uint32_t)(now - last_sd_log_ms) >= SD_LOG_PERIOD_MS)
    {
        /*
         * Προχωράμε κατά μία σταθερή περίοδο ώστε να μειώνεται
         * το scheduler drift.
         */
        last_sd_log_ms = now;

        if (SDLogger_IsReady())
        {
            SDLogRecord_t record;

            /*
             * Timestamp του record.
             */
            record.timestamp_ms = now;

            /*
             * Barometer.
             */
            record.pressure_pa =
                s->pressure_pa;

            record.baro_altitude_m =
                s->baro_altitude_m;

            /*
             * Filter output.
             */
            record.estimated_altitude_m =
                VerticalFilter_GetAltitude(&vertical_filter);

            record.estimated_velocity_mps =
                VerticalFilter_GetVelocity(&vertical_filter);

            /*
             * Accelerometer.
             */
            record.accel_x_g = s->imu.acc_x_g;
            record.accel_y_g = s->imu.acc_y_g;
            record.accel_z_g = s->imu.acc_z_g;

            /*
             * Gyroscope.
             */
            record.gyro_x_dps = s->imu.gyro_x_dps;
            record.gyro_y_dps = s->imu.gyro_y_dps;
            record.gyro_z_dps = s->imu.gyro_z_dps;

            /*
             * GPS.
             *
             * Αν δεν υπάρχει fix, οι συντεταγμένες μπορεί να είναι
             * μηδέν ή οι τελευταίες γνωστές τιμές.
             */
            record.gps_fix =
                s->gps.valid_fix;

            record.gps_satellites =
                s->gps.satellites;

            record.gps_latitude_deg =
                s->gps.latitude_deg;

            record.gps_longitude_deg =
                s->gps.longitude_deg;

            record.gps_altitude_m =
                s->gps.altitude_m;

            /*
             * Γράφουμε το record.
             *
             * Δεν κάνουμε printf για κάθε επιτυχημένο write, γιατί
             * αυτό θα επιβάρυνε πολύ το loop.
             */
            SDLoggerStatus_t status =
                SDLogger_WriteRecord(&record);

            if (status != SD_LOGGER_OK)
            {
                printf("SD write error: %u\r\n",
                       (unsigned int)status);
            }
        }
    }

    /*
     * ============================================================
     * SD SYNC TASK — 1 Hz
     * ============================================================
     *
     * Το f_write() μπορεί να αφήσει δεδομένα σε buffers.
     *
     * Το f_sync() ζητά να αποθηκευτούν πραγματικά στην κάρτα.
     *
     * Το κάνουμε μόνο μία φορά το δευτερόλεπτο, επειδή το sync
     * μπορεί να είναι αργό.
     */

    if ((uint32_t)(now - last_sd_sync_ms) >= SD_SYNC_PERIOD_MS)
    {
    	last_sd_sync_ms = now;

        if (SDLogger_IsReady())
        {
            SDLoggerStatus_t status = SDLogger_Sync();

            if (status != SD_LOGGER_OK)
            {
                printf("SD sync error: %u\r\n",
                       (unsigned int)status);
            }
        }
    }
    /*
     * ========================================================
     * USB DEBUG TASK — 2 Hz
     * ========================================================
     */

    if ((uint32_t)(now - last_debug_ms) >= DEBUG_PERIOD_MS)
    {
        last_debug_ms += DEBUG_PERIOD_MS;

        float acceleration_input =
            (s->imu.acc_z_g - accel_z_rest_g)
            * GRAVITY_MPS2;

        printf("\r\n");
        printf("========== FLIGHT DATA ==========\r\n");

        printf("Time            = %lu ms\r\n",
               (unsigned long)now);

        printf("Baro Altitude   = %.3f m\r\n",
               s->baro_altitude_m);

        printf("Estimated Alt   = %.3f m\r\n",
               VerticalFilter_GetAltitude(
                   &vertical_filter));

        printf("Estimated Vel   = %.3f m/s\r\n",
               VerticalFilter_GetVelocity(
                   &vertical_filter));

        printf("Accel X         = %.4f g\r\n",
               s->imu.acc_x_g);

        printf("Accel Y         = %.4f g\r\n",
               s->imu.acc_y_g);

        printf("Accel Z         = %.4f g\r\n",
               s->imu.acc_z_g);

        printf("Accel Z Rest    = %.4f g\r\n",
               accel_z_rest_g);

        printf("Accel Input     = %.4f m/s^2\r\n",
               acceleration_input);

        printf("Gyro X          = %.3f dps\r\n",
               s->imu.gyro_x_dps);

        printf("Gyro Y          = %.3f dps\r\n",
               s->imu.gyro_y_dps);

        printf("Gyro Z          = %.3f dps\r\n",
               s->imu.gyro_z_dps);

        printf("GPS Fix         = %u\r\n",
               s->gps.valid_fix);

        printf("GPS Satellites  = %u\r\n",
               s->gps.satellites);

        printf("GPS Latitude    = %.6f\r\n",
               s->gps.latitude_deg);

        printf("GPS Longitude   = %.6f\r\n",
               s->gps.longitude_deg);

        printf("=================================\r\n");
    }
}

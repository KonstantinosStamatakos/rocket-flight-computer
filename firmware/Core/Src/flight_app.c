#include "flight_app.h"
#include "app_config.h"
#include "sensors.h"
#include "vertical_filter.h"
#include "main.h"
#include "sd_logger.h"

#include <stdio.h>
#include <stdint.h>

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


static VerticalFilter_t vertical_filter;

static float accel_z_rest_g = 1.0f;

static void FlightApp_CalibrateAccelerometer(void)
{
    float sum_z_g = 0.0f;
    uint32_t valid_samples = 0U;

    printf("Keep the board still for accelerometer calibration...\r\n");

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
    
    sensors_ok = Sensors_Init(&hi2c1, &huart2);

    printf("Sensor initialization status = %u\r\n",
           sensors_ok);

    FlightApp_CalibrateAccelerometer();

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
        printf("SD logger initialization failed: %u\r\n",
               (unsigned int)sd_status);
    }

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

        float dt_s = (float)elapsed_ms * 0.001f;

        last_filter_ms = now;

        if (s->imu_ok && dt_s > 0.0f && dt_s < 0.050f)
        {
            float accel_z_mps2 =
                (s->imu.acc_z_g - accel_z_rest_g)
                * GRAVITY_MPS2;

            if (accel_z_mps2 > -ACCEL_DEADBAND_MPS2 &&
                accel_z_mps2 <  ACCEL_DEADBAND_MPS2)
            {
                accel_z_mps2 = 0.0f;
            }

            VerticalFilter_Predict(&vertical_filter,
                                   accel_z_mps2,
                                   dt_s);
        }

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
     */

    if ((uint32_t)(now - last_sd_log_ms) >= SD_LOG_PERIOD_MS)
    {
        last_sd_log_ms = now;

        if (SDLogger_IsReady())
        {
            SDLogRecord_t record;

            record.timestamp_ms = now;

            record.pressure_pa =
                s->pressure_pa;

            record.baro_altitude_m =
                s->baro_altitude_m;
            
            record.estimated_altitude_m =
                VerticalFilter_GetAltitude(&vertical_filter);

            record.estimated_velocity_mps =
                VerticalFilter_GetVelocity(&vertical_filter);
            
            record.accel_x_g = s->imu.acc_x_g;
            record.accel_y_g = s->imu.acc_y_g;
            record.accel_z_g = s->imu.acc_z_g;

            record.gyro_x_dps = s->imu.gyro_x_dps;
            record.gyro_y_dps = s->imu.gyro_y_dps;
            record.gyro_z_dps = s->imu.gyro_z_dps;

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

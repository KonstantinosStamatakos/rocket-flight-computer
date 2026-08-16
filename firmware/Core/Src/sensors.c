/*
 * sensors.c
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */

#include "sensors.h"
#include "app_config.h"
#include "bmp581.h"

#include <math.h>
#include <string.h>

/*
 * ============================================================
 * DRIVER INSTANCES
 * ============================================================
 */

static BMP581_t bmp581;
static LSM6DSO32_t imu_device;

static SensorsData_t sensors;

/*
 * Αποθηκεύουμε το UART handle του GPS ώστε να μπορούμε να
 * ελέγξουμε αν το interrupt προήλθε από το σωστό UART.
 */
static UART_HandleTypeDef *gps_uart = NULL;

/*
 * Το UART interrupt λαμβάνει ένα byte κάθε φορά.
 */
static uint8_t gps_rx_byte = 0U;

/*
 * ============================================================
 * BAROMETER STATE
 * ============================================================
 */

static float startup_pressure_hpa = 0.0f;
static float filtered_pressure_hpa = 0.0f;

static uint8_t altitude_zero_ready = 0U;
static uint8_t pressure_filter_ready = 0U;

/*
 * ============================================================
 * PRESSURE TO RELATIVE ALTITUDE
 * ============================================================
 *
 * Υπολογίζει altitude σε σχέση με το pressure που είχαμε
 * κατά την εκκίνηση.
 *
 * Επομένως:
 *
 * startup altitude ≈ 0 m
 *
 * Δεν είναι absolute altitude από τη στάθμη της θάλασσας.
 */

static float Sensors_CalculateRelativeAltitude(float pressure_hpa,
                                               float reference_hpa)
{
    if (pressure_hpa <= 0.0f || reference_hpa <= 0.0f)
    {
        return 0.0f;
    }

    return 44330.0f *
           (1.0f - powf(pressure_hpa / reference_hpa, 0.1903f));
}

/*
 * ============================================================
 * SENSORS INITIALIZATION
 * ============================================================
 */

uint8_t Sensors_Init(I2C_HandleTypeDef *hi2c,
                     UART_HandleTypeDef *huart)
{
    /*
     * Μηδενίζουμε ολόκληρη τη δομή, ώστε να μην περιέχει
     * τυχαίες τιμές από RAM.
     */
    memset(&sensors, 0, sizeof(sensors));

    /*
     * ---------------- BMP581 ----------------
     */

    sensors.bmp_ok =
        BMP581_Init(&bmp581,
                    hi2c,
                    BMP581_I2C_ADDR);

    /*
     * ---------------- LSM6DSO32 ----------------
     */

    sensors.imu_ok =
        LSM6DSO32_Init(&imu_device,
                       hi2c,
                       LSM6DSO32_I2C_ADDR);

    /*
     * ---------------- GPS ----------------
     */

    gps_uart = huart;

    GPS_Init(&sensors.gps, huart);

    sensors.gps_ok = 1U;

    /*
     * Ξεκινάμε interrupt-based λήψη ενός byte.
     *
     * Όταν φτάσει το byte, το HAL θα καλέσει:
     *
     * HAL_UART_RxCpltCallback()
     *
     * και από εκεί θα καλέσουμε:
     *
     * Sensors_GpsRxComplete()
     */
    if (HAL_UART_Receive_IT(gps_uart,
                           &gps_rx_byte,
                           1U) != HAL_OK)
    {
        sensors.gps_ok = 0U;
    }

    /*
     * ========================================================
     * BAROMETER STARTUP CALIBRATION
     * ========================================================
     *
     * Αφήνουμε το sensor να σταθεροποιηθεί και παίρνουμε πολλά
     * samples ώστε να βρούμε το startup pressure.
     *
     * Κατά τη διάρκεια αυτής της διαδικασίας η πλακέτα πρέπει
     * να παραμένει ακίνητη.
     */

    if (sensors.bmp_ok)
    {
        float pressure_sum_hpa = 0.0f;
        uint32_t valid_samples = 0U;

        HAL_Delay(1000U);

        for (uint32_t i = 0U; i < 50U; i++)
        {
            float temperature_c = 0.0f;
            float pressure_pa = 0.0f;

            if (BMP581_ReadTempPressure(&bmp581,
                                        &temperature_c,
                                        &pressure_pa))
            {
                pressure_sum_hpa += pressure_pa / 100.0f;
                valid_samples++;
            }

            HAL_Delay(20U);
        }

        if (valid_samples > 0U)
        {
            startup_pressure_hpa =
                pressure_sum_hpa / (float)valid_samples;

            filtered_pressure_hpa = startup_pressure_hpa;

            altitude_zero_ready = 1U;
            pressure_filter_ready = 1U;
        }
        else
        {
            sensors.bmp_ok = 0U;
        }
    }

    /*
     * BMP581 και IMU θεωρούνται critical sensors.
     *
     * Το GPS μπορεί να μην έχει fix, αλλά το flight computer
     * πρέπει να μπορεί να λειτουργήσει.
     */
    return (sensors.bmp_ok && sensors.imu_ok) ? 1U : 0U;
}

/*
 * ============================================================
 * IMU UPDATE
 * ============================================================
 */

uint8_t Sensors_UpdateImu(void)
{
    if (!sensors.imu_ok)
    {
        return 0U;
    }

    if (!LSM6DSO32_ReadAll(&imu_device,
                           &sensors.imu))
    {
        sensors.imu_new_data = 0U;
        return 0U;
    }

    sensors.imu_timestamp_ms = HAL_GetTick();
    sensors.imu_new_data = 1U;

    return 1U;
}

/*
 * ============================================================
 * BAROMETER UPDATE
 * ============================================================
 */

uint8_t Sensors_UpdateBaro(void)
{
    float temperature_c = 0.0f;
    float pressure_pa = 0.0f;

    if (!sensors.bmp_ok)
    {
        return 0U;
    }

    if (!BMP581_ReadTempPressure(&bmp581,
                                 &temperature_c,
                                 &pressure_pa))
    {
        sensors.baro_new_data = 0U;
        return 0U;
    }

    sensors.bmp_temp_c = temperature_c;
    sensors.pressure_pa = pressure_pa;
    sensors.pressure_hpa = pressure_pa / 100.0f;

    /*
     * Αρχικοποίηση του low-pass filter στο πρώτο sample.
     */
    if (!pressure_filter_ready)
    {
        filtered_pressure_hpa = sensors.pressure_hpa;
        pressure_filter_ready = 1U;
    }
    else
    {
        filtered_pressure_hpa =
            ((1.0f - BARO_FILTER_ALPHA) *
             filtered_pressure_hpa)
            +
            (BARO_FILTER_ALPHA *
             sensors.pressure_hpa);
    }

    /*
     * Μετατροπή του filtered pressure σε relative altitude.
     */
    if (altitude_zero_ready)
    {
        sensors.baro_altitude_m =
            Sensors_CalculateRelativeAltitude(
                filtered_pressure_hpa,
                startup_pressure_hpa);
    }
    else
    {
        sensors.baro_altitude_m = 0.0f;
    }

    sensors.baro_timestamp_ms = HAL_GetTick();
    sensors.baro_new_data = 1U;

    return 1U;
}

/*
 * ============================================================
 * GPS UART INTERRUPT HANDLING
 * ============================================================
 */

void Sensors_GpsRxComplete(UART_HandleTypeDef *huart)
{
    /*
     * Βεβαιωνόμαστε ότι το interrupt προέρχεται από το GPS UART.
     */
    if (huart != gps_uart)
    {
        return;
    }

    /*
     * Δίνουμε το byte στον NMEA parser.
     */
    GPS_ProcessByte(&sensors.gps, gps_rx_byte);

    /*
     * Όταν έχει ολοκληρωθεί μία NMEA sentence, προσπαθούμε
     * να την κάνουμε parse.
     */
    if (sensors.gps.sentence_ready)
    {
        if (GPS_ParseLatest(&sensors.gps))
        {
            sensors.gps_timestamp_ms = HAL_GetTick();
            sensors.gps_new_data = 1U;
        }

        /*
         * Η sentence καταναλώθηκε.
         */
        sensors.gps.sentence_ready = 0U;
    }

    /*
     * Το HAL UART interrupt λαμβάνει μόνο ένα byte.
     *
     * Επομένως πρέπει να το ενεργοποιούμε ξανά μετά από κάθε
     * ολοκληρωμένο byte.
     */
    HAL_UART_Receive_IT(gps_uart,
                        &gps_rx_byte,
                        1U);
}

/*
 * ============================================================
 * SENSOR DATA ACCESS
 * ============================================================
 */

SensorsData_t *Sensors_GetData(void)
{
    return &sensors;
}

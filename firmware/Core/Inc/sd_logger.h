#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <stdint.h>

/*
 * ============================================================
 * SD LOGGER STATUS
 * ============================================================
 */

typedef enum
{
    SD_LOGGER_OK = 0,
    SD_LOGGER_MOUNT_ERROR,
    SD_LOGGER_OPEN_ERROR,
    SD_LOGGER_WRITE_ERROR,
    SD_LOGGER_NOT_READY
} SDLoggerStatus_t;

/*
 * ============================================================
 * SD LOG RECORD
 * ============================================================
 */

typedef struct
{
    uint32_t timestamp_ms;

    float pressure_pa;
    float baro_altitude_m;

    float estimated_altitude_m;
    float estimated_velocity_mps;

    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;

    uint8_t gps_fix;
    uint8_t gps_satellites;

    float gps_latitude_deg;
    float gps_longitude_deg;
    float gps_altitude_m;

} SDLogRecord_t;


SDLoggerStatus_t SDLogger_Init(void);

SDLoggerStatus_t SDLogger_WriteRecord(const SDLogRecord_t *record);

SDLoggerStatus_t SDLogger_Sync(void);

void SDLogger_Close(void);

uint8_t SDLogger_IsReady(void);

const char *SDLogger_GetFilename(void);

#endif

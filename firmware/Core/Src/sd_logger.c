#include "sd_logger.h"

#include "fatfs.h"
#include "ff.h"

#include <stdio.h>
#include <string.h>

extern FATFS SDFatFS;
extern char SDPath[4];

static FIL log_file;

static uint8_t logger_ready = 0U;

static char log_filename[16] = {0};

static char line_buffer[384];

/*
 * ============================================================
 * INTERNAL WRITE FUNCTION
 * ============================================================
 */

static SDLoggerStatus_t SDLogger_WriteBytes(const char *data,
                                            UINT length)
{
    UINT bytes_written = 0U;

    if (!logger_ready)
    {
        return SD_LOGGER_NOT_READY;
    }

    FRESULT result = f_write(&log_file,
                             data,
                             length,
                             &bytes_written);

    if (result != FR_OK)
    {
        return SD_LOGGER_WRITE_ERROR;
    }

    if (bytes_written != length)
    {
        return SD_LOGGER_WRITE_ERROR;
    }

    return SD_LOGGER_OK;
}

/*
 * ============================================================
 * INITIALIZATION
 * ============================================================
 */

SDLoggerStatus_t SDLogger_Init(void)
{
    FRESULT result;

    logger_ready = 0U;

    result = f_mount(&SDFatFS, SDPath, 1U);

    if (result != FR_OK)
    {
        return SD_LOGGER_MOUNT_ERROR;
    }
  
    uint8_t filename_found = 0U;

    for (uint32_t index = 0U; index < 100U; index++)
    {
        snprintf(log_filename,
                 sizeof(log_filename),
                 "FLIGHT%02lu.CSV",
                 (unsigned long)index);

        result = f_stat(log_filename, NULL);

        if (result == FR_NO_FILE)
        {
            filename_found = 1U;
            break;
        }

        if (result != FR_OK)
        {
            return SD_LOGGER_OPEN_ERROR;
        }
    }

    if (!filename_found)
    {
        return SD_LOGGER_OPEN_ERROR;
    }


    result = f_open(&log_file,
                    log_filename,
                    FA_CREATE_NEW | FA_WRITE);

    if (result != FR_OK)
    {
        return SD_LOGGER_OPEN_ERROR;
    }

    logger_ready = 1U;

    /*
     * --------------------------------------------------------
     * 4. CSV header
     * --------------------------------------------------------
     */
    const char *header =
        "time_ms,"
        "pressure_pa,"
        "baro_altitude_m,"
        "estimated_altitude_m,"
        "estimated_velocity_mps,"
        "accel_x_g,"
        "accel_y_g,"
        "accel_z_g,"
        "gyro_x_dps,"
        "gyro_y_dps,"
        "gyro_z_dps,"
        "gps_fix,"
        "gps_satellites,"
        "gps_latitude_deg,"
        "gps_longitude_deg,"
        "gps_altitude_m\r\n";

    SDLoggerStatus_t write_status =
        SDLogger_WriteBytes(header, (UINT)strlen(header));

    if (write_status != SD_LOGGER_OK)
    {
        f_close(&log_file);
        logger_ready = 0U;
        return write_status;
    }

    result = f_sync(&log_file);

    if (result != FR_OK)
    {
        f_close(&log_file);
        logger_ready = 0U;
        return SD_LOGGER_WRITE_ERROR;
    }

    return SD_LOGGER_OK;
}

/*
 * ============================================================
 * WRITE ONE CSV RECORD
 * ============================================================
 */

SDLoggerStatus_t SDLogger_WriteRecord(const SDLogRecord_t *record)
{
    if (!logger_ready)
    {
        return SD_LOGGER_NOT_READY;
    }

    if (record == NULL)
    {
        return SD_LOGGER_WRITE_ERROR;
    }

    int length = snprintf(
        line_buffer,
        sizeof(line_buffer),

        "%lu,"
        "%.2f,"
        "%.4f,"
        "%.4f,"
        "%.4f,"
        "%.5f,"
        "%.5f,"
        "%.5f,"
        "%.4f,"
        "%.4f,"
        "%.4f,"
        "%u,"
        "%u,"
        "%.7f,"
        "%.7f,"
        "%.3f\r\n",

        (unsigned long)record->timestamp_ms,

        record->pressure_pa,
        record->baro_altitude_m,

        record->estimated_altitude_m,
        record->estimated_velocity_mps,

        record->accel_x_g,
        record->accel_y_g,
        record->accel_z_g,

        record->gyro_x_dps,
        record->gyro_y_dps,
        record->gyro_z_dps,

        record->gps_fix,
        record->gps_satellites,

        record->gps_latitude_deg,
        record->gps_longitude_deg,
        record->gps_altitude_m
    );

    if (length < 0)
    {
        return SD_LOGGER_WRITE_ERROR;
    }

    if ((size_t)length >= sizeof(line_buffer))
    {
        return SD_LOGGER_WRITE_ERROR;
    }

    return SDLogger_WriteBytes(line_buffer, (UINT)length);
}

/*
 * ============================================================
 * SYNC
 * ============================================================
 */

SDLoggerStatus_t SDLogger_Sync(void)
{
    if (!logger_ready)
    {
        return SD_LOGGER_NOT_READY;
    }

    if (f_sync(&log_file) != FR_OK)
    {
        return SD_LOGGER_WRITE_ERROR;
    }

    return SD_LOGGER_OK;
}

/*
 * ============================================================
 * CLOSE
 * ============================================================
 */

void SDLogger_Close(void)
{
    if (logger_ready)
    {
        f_sync(&log_file);
      
        f_close(&log_file);

        logger_ready = 0U;
    }
    f_mount(NULL, SDPath, 1U);
}

uint8_t SDLogger_IsReady(void)
{
    return logger_ready;
}

const char *SDLogger_GetFilename(void)
{
    return log_filename;
}

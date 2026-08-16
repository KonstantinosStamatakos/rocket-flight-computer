/*
 * sd_logger.c
 *
 *  Created on: Aug 9, 2026
 *      Author: deanstamatakos
 */

#include "sd_logger.h"

#include "fatfs.h"
#include "ff.h"

#include <stdio.h>
#include <string.h>

/*
 * ============================================================
 * FATFS OBJECTS
 * ============================================================
 *
 * Τα SDFatFS και SDPath δημιουργούνται συνήθως από το CubeMX
 * μέσα στο fatfs.c.
 *
 * Δηλώνονται extern ώστε να χρησιμοποιήσουμε τα ίδια objects
 * και όχι να δημιουργήσουμε δεύτερα.
 */

extern FATFS SDFatFS;
extern char SDPath[4];

/*
 * Το FIL αντιπροσωπεύει το ανοιχτό CSV αρχείο.
 */
static FIL log_file;

/*
 * 1 όταν:
 * - έγινε mount
 * - άνοιξε σωστά το αρχείο
 * - γράφτηκε το CSV header
 */
static uint8_t logger_ready = 0U;

/*
 * Αποθηκεύουμε το όνομα του αρχείου που δημιουργήθηκε.
 *
 * Παράδειγμα:
 * "FLIGHT03.CSV"
 */
static char log_filename[16] = {0};

/*
 * Buffer όπου δημιουργούμε κάθε CSV γραμμή πριν την γράψουμε.
 *
 * Χρησιμοποιούμε snprintf() αντί για πολλά μικρά f_write(),
 * ώστε ολόκληρη η γραμμή να γράφεται με μία κλήση.
 */
static char line_buffer[384];

/*
 * ============================================================
 * INTERNAL WRITE FUNCTION
 * ============================================================
 *
 * Γράφει ακριβώς len bytes στην SD.
 *
 * Ελέγχουμε:
 * - το αποτέλεσμα του f_write()
 * - αν γράφτηκε ολόκληρο το requested μήκος
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

    /*
     * Αρχικά θεωρούμε ότι ο logger δεν είναι έτοιμος.
     */
    logger_ready = 0U;

    /*
     * --------------------------------------------------------
     * 1. Mount filesystem
     * --------------------------------------------------------
     *
     * Το 1 σημαίνει ότι το FatFs πρέπει να κάνει mount αμέσως.
     */
    result = f_mount(&SDFatFS, SDPath, 1U);

    if (result != FR_OK)
    {
        return SD_LOGGER_MOUNT_ERROR;
    }

    /*
     * --------------------------------------------------------
     * 2. Βρες διαθέσιμο filename
     * --------------------------------------------------------
     *
     * Δεν θέλουμε να διαγράψουμε προηγούμενο flight log.
     *
     * Ελέγχουμε διαδοχικά:
     *
     * FLIGHT00.CSV
     * FLIGHT01.CSV
     * ...
     * FLIGHT99.CSV
     */
    uint8_t filename_found = 0U;

    for (uint32_t index = 0U; index < 100U; index++)
    {
        snprintf(log_filename,
                 sizeof(log_filename),
                 "FLIGHT%02lu.CSV",
                 (unsigned long)index);

        /*
         * f_stat() επιστρέφει FR_NO_FILE όταν το αρχείο
         * δεν υπάρχει. Αυτό είναι το filename που θέλουμε.
         */
        result = f_stat(log_filename, NULL);

        if (result == FR_NO_FILE)
        {
            filename_found = 1U;
            break;
        }

        /*
         * FR_OK σημαίνει ότι το filename υπάρχει ήδη,
         * οπότε συνεχίζουμε στο επόμενο.
         */
        if (result != FR_OK)
        {
            return SD_LOGGER_OPEN_ERROR;
        }
    }

    if (!filename_found)
    {
        return SD_LOGGER_OPEN_ERROR;
    }

    /*
     * --------------------------------------------------------
     * 3. Άνοιγμα καινούριου αρχείου
     * --------------------------------------------------------
     *
     * FA_CREATE_NEW:
     * αποτυγχάνει αν υπάρχει ήδη, προστατεύοντας προηγούμενα logs.
     *
     * FA_WRITE:
     * ανοίγει το αρχείο για εγγραφή.
     */
    result = f_open(&log_file,
                    log_filename,
                    FA_CREATE_NEW | FA_WRITE);

    if (result != FR_OK)
    {
        return SD_LOGGER_OPEN_ERROR;
    }

    /*
     * Από αυτό το σημείο και μετά επιτρέπεται η χρήση της
     * internal SDLogger_WriteBytes().
     */
    logger_ready = 1U;

    /*
     * --------------------------------------------------------
     * 4. CSV header
     * --------------------------------------------------------
     *
     * Η πρώτη γραμμή περιέχει τα ονόματα των στηλών.
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

    /*
     * Κάνουμε sync το header ώστε το αρχείο να υπάρχει άμεσα
     * στην κάρτα ακόμη και αν γίνει reset λίγο αργότερα.
     */
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

    /*
     * Μετατρέπουμε όλα τα πεδία σε μία CSV γραμμή.
     *
     * Παράδειγμα:
     *
     * 15420,101325.4,0.24,0.20,0.03,...
     */
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

    /*
     * snprintf() επιστρέφει:
     *
     * < 0:
     *   formatting error
     *
     * >= sizeof(buffer):
     *   η γραμμή δεν χώρεσε και κόπηκε
     */
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
        /*
         * Προσπαθούμε πρώτα να αποθηκεύσουμε ό,τι εκκρεμεί.
         */
        f_sync(&log_file);

        /*
         * Κλείνουμε το αρχείο ώστε να ενημερωθεί σωστά το
         * filesystem metadata.
         */
        f_close(&log_file);

        logger_ready = 0U;
    }

    /*
     * Unmount του filesystem.
     */
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

/*
 * sd_logger.h
 *
 *  Created on: Aug 9, 2026
 *      Author: deanstamatakos
 */
#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <stdint.h>

/*
 * ============================================================
 * SD LOGGER STATUS
 * ============================================================
 *
 * Οι τιμές αυτές μας επιτρέπουν να γνωρίζουμε:
 * - αν έγινε mount η κάρτα
 * - αν άνοιξε το αρχείο
 * - αν έγινε κάποιο write error
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
 *
 * Κάθε φορά που θέλουμε να γράψουμε ένα sample στην SD,
 * δημιουργούμε μία τέτοια δομή.
 *
 * Η δομή περιέχει όλα τα σημαντικά flight δεδομένα της
 * συγκεκριμένης χρονικής στιγμής.
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

/*
 * Κάνει mount την κάρτα και ανοίγει ένα νέο CSV αρχείο.
 *
 * Δημιουργεί αυτόματα:
 *
 * FLIGHT00.CSV
 * FLIGHT01.CSV
 * FLIGHT02.CSV
 * ...
 *
 * ώστε να μην αντικαθιστά προηγούμενη καταγραφή.
 */
SDLoggerStatus_t SDLogger_Init(void);

/*
 * Γράφει ένα record στο CSV.
 *
 * Δεν κάνει f_sync() σε κάθε γραμμή, γιατί αυτό θα προκαλούσε
 * μεγάλες καθυστερήσεις.
 */
SDLoggerStatus_t SDLogger_WriteRecord(const SDLogRecord_t *record);

/*
 * Αναγκάζει τα δεδομένα που βρίσκονται στα internal buffers
 * του FatFs/SD να γραφτούν στην κάρτα.
 */
SDLoggerStatus_t SDLogger_Sync(void);

/*
 * Κλείνει σωστά το αρχείο και κάνει unmount την κάρτα.
 */
void SDLogger_Close(void);

/*
 * Επιστρέφει 1 όταν ο logger είναι έτοιμος.
 */
uint8_t SDLogger_IsReady(void);

/*
 * Επιστρέφει το όνομα του αρχείου που δημιουργήθηκε.
 */
const char *SDLogger_GetFilename(void);

#endif

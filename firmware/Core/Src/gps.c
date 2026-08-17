#include "gps.h"
#include <string.h>
#include <stdlib.h>


static float GPS_NmeaToDecimalDegrees(const char *nmea, char hemi)
{
    if (nmea == NULL || nmea[0] == '\0')
        return 0.0f;

    /*
     * NMEA latitude/longitude format:
     *   ddmm.mmmm  or  dddmm.mmmm
     */
    float val = (float)atof(nmea);
    int degrees = (int)(val / 100.0f);
    float minutes = val - (degrees * 100.0f);

    float decimal = degrees + (minutes / 60.0f);

    if (hemi == 'S' || hemi == 'W')
        decimal = -decimal;

    return decimal;
}

void GPS_Init(GPS_t *gps, UART_HandleTypeDef *huart)
{
    memset(gps, 0, sizeof(GPS_t));
    gps->huart = huart;
}

void GPS_ProcessByte(GPS_t *gps, uint8_t byte)
{
    if (byte == '\r')
        return;

    if (byte == '\n')
    {
        if (gps->idx > 0)
        {
            gps->line[gps->idx] = '\0';

            strncpy(gps->sentence, gps->line, GPS_LINE_MAX_LEN - 1);
            gps->sentence[GPS_LINE_MAX_LEN - 1] = '\0';
            gps->sentence_ready = 1;
        }

        gps->idx = 0;
        return;
    }

    if (gps->idx < (GPS_LINE_MAX_LEN - 1))
    {
        gps->line[gps->idx++] = (char)byte;
    }
    else
    {
        /* overflow protection: reset line */
        gps->idx = 0;
    }
}

uint8_t GPS_ReadSentenceBlocking(GPS_t *gps, uint32_t timeout_ms)
{
    uint8_t ch;
    uint32_t start = HAL_GetTick();

    gps->sentence_ready = 0;

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (HAL_UART_Receive(gps->huart, &ch, 1, 10) == HAL_OK)
        {
            GPS_ProcessByte(gps, ch);

            if (gps->sentence_ready)
                return 1;
        }
    }

    return 0;
}

uint8_t GPS_ParseLatest(GPS_t *gps)
{
    char buf[GPS_LINE_MAX_LEN];
    char *token;
    char *saveptr = NULL;

    strncpy(buf, gps->sentence, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    token = strtok_r(buf, ",", &saveptr);
    if (token == NULL)
        return 0;

    /* ---------------- GGA sentence ---------------- */
    if ((strcmp(token, "$GPGGA") == 0) || (strcmp(token, "$GNGGA") == 0))
    {
        char *fields[15] = {0};
        int i = 0;

        while (i < 15 && (fields[i] = strtok_r(NULL, ",", &saveptr)) != NULL)
            i++;

        if (fields[1] && fields[2] && fields[3] && fields[4] &&
            fields[5] && fields[6] && fields[8])
        {
            char ns = fields[2][0];
            char ew = fields[4][0];
            int fix_quality = atoi(fields[5]);

            gps->latitude_deg  = GPS_NmeaToDecimalDegrees(fields[1], ns);
            gps->longitude_deg = GPS_NmeaToDecimalDegrees(fields[3], ew);
            gps->valid_fix     = (fix_quality > 0) ? 1U : 0U;
            gps->satellites    = (uint8_t)atoi(fields[6]);
            gps->altitude_m    = (float)atof(fields[8]);

            return 1;
        }
    }

    /* ---------------- RMC sentence ---------------- */
    if ((strcmp(token, "$GPRMC") == 0) || (strcmp(token, "$GNRMC") == 0))
    {
        char *fields[12] = {0};
        int i = 0;

        while (i < 12 && (fields[i] = strtok_r(NULL, ",", &saveptr)) != NULL)
            i++;

        if (fields[1] && fields[2] && fields[3] &&
            fields[4] && fields[5] && fields[6])
        {
            char status = fields[1][0];
            char ns = fields[3][0];
            char ew = fields[5][0];

            gps->valid_fix     = (status == 'A') ? 1U : 0U;
            gps->latitude_deg  = GPS_NmeaToDecimalDegrees(fields[2], ns);
            gps->longitude_deg = GPS_NmeaToDecimalDegrees(fields[4], ew);
            gps->speed_knots   = (float)atof(fields[6]);

            return 1;
        }
    }

    return 0;
}

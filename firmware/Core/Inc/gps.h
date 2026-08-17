#ifndef GPS_H
#define GPS_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define GPS_LINE_MAX_LEN  128

typedef struct
{
    UART_HandleTypeDef *huart;

    /* Line-building buffer while reading UART bytes */
    char line[GPS_LINE_MAX_LEN];
    uint16_t idx;

    /* Last full sentence received */
    uint8_t sentence_ready;
    char sentence[GPS_LINE_MAX_LEN];

    /* Latest decoded values */
    uint8_t valid_fix;
    uint8_t satellites;

    float latitude_deg;
    float longitude_deg;
    float altitude_m;
    float speed_knots;
} GPS_t;

void GPS_Init(GPS_t *gps, UART_HandleTypeDef *huart);
void GPS_ProcessByte(GPS_t *gps, uint8_t byte);
uint8_t GPS_ReadSentenceBlocking(GPS_t *gps, uint32_t timeout_ms);
uint8_t GPS_ParseLatest(GPS_t *gps);

#endif

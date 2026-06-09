#ifndef __GPS_H
#define __GPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define GPS_NMEA_SENTENCE_MAX  96U

void GPS_Init(void);
void GPS_Update(void);
void GPS_GetLatestSentence(char *buffer, uint16_t buffer_size);
uint8_t GPS_HasSentence(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPS_H */

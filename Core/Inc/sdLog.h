/*
 * sdLog.h
 *
 *  Created on: 11 Nov 2026
 *      Author: matpj
 */

#ifndef INC_SDLOG_H_
#define INC_SDLOG_H_

#include <stdint.h>
#include <stdbool.h>
#include "ff.h"

typedef struct {
    uint16_t year;
    uint8_t  month, day, hour, min, sec;
} sdlog_time_t;

/* mounts the volume and writes a "=== SD BOOT Success ===" marker line */
FRESULT sdlog_init(void);

/* log a plain string (no format processing, '%' is safe) */
FRESULT sdlog_str(const char *s);

/* log a printf-style message, e.g. sdlog("ADC = %u", adc); */
FRESULT sdlog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Time source hook. Default (weak) returns false = no RTC, so the logger
 * uses uptime (D<days> hh:mm:ss.mmm). Define your own to supply real time:
 * fill *t and return true. */
bool sdlog_rtc_get(sdlog_time_t *t);

#endif /* INC_SDLOG_H_ */

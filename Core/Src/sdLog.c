/*
 * sdLog.c
 *
 *  Created on: 11 Nov 2026
 *      Author: matpj
 */


/*
 * sdlog.c
 *
 *  SD card logger on top of FatFs.
 *  Each call: open LOG.TXT (append) -> write one line -> close.
 *  Closing every time flushes data and the directory entry, so a power cut
 *  loses at most the line being written.
 */

#include "sdLog.h"
#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <FatFs_debug.h>

#define SDLOG_FILE      "LOG.TXT"
#define SDLOG_LINE_MAX  512

static FATFS  fs;
static FIL    fil;
static uint8_t mounted = 0;

/* Default: no RTC present.  TODO Override this function elsewhere once we have an RTC. */
__weak bool sdlog_rtc_get(sdlog_time_t *t)
{
    (void)t;
    return false;
}

/* writes the timestamp text into out, returns its length */
static int make_stamp(char *out, size_t size)
{
    sdlog_time_t t;

    if (sdlog_rtc_get(&t))
    {	//RTC present
        return snprintf(out, size, "%04u-%02u-%02u %02u:%02u:%02u", t.year, t.month, t.day, t.hour, t.min, t.sec); // untested yet
    }

    /* no RTC:
     * day 0, time 0 at power-up, then count up from the tick */
    uint32_t ms = HAL_GetTick();
    uint32_t s  = ms / 1000;
    return snprintf(out, size, "D%lu %02lu:%02lu:%02lu.%03lu",
                    (unsigned long)(s / 86400),
                    (unsigned long)((s / 3600) % 24),
                    (unsigned long)((s / 60) % 60),
                    (unsigned long)(s % 60),
                    (unsigned long)(ms % 1000));
}



FRESULT sdlog_init(void)
{
    if (mounted == 1) // avoid repeated UART messages.
    		return FR_OK;

	FRESULT fr = f_mount(&fs, "", 1);   /* 1 = mount-now or Force-mounted */
	if (fr != FR_OK) return fr;
	mounted = 1;
	return sdlog_str("=== SD BOOT Success ===");
}



static FRESULT log_va_list(const char *fmt, va_list ap)
{
    if (!mounted) return FR_NOT_READY;

    static char line[SDLOG_LINE_MAX];
    int n = make_stamp(line, sizeof line);
    n += snprintf(line + n, sizeof line - n, ": ");
    n += vsnprintf(line + n, sizeof line - n, fmt, ap);
    /* n=a cursor. It tracks how many characters are in line so far,
     * so the next write knows where to start;
     * snprintf returns the length of the text it wrote
     * -(not counting the \0) */

    if (n > (int)sizeof line - 3) n = sizeof line - 3;
    /* truncated: leave room for \r\n */
    line[n++] = '\r';
    line[n++] = '\n';

    FRESULT fr = f_open(&fil, SDLOG_FILE, FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) return fr;

    UINT bw;
    fr = f_write(&fil, line, (UINT)n, &bw);
    FRESULT fc = f_close(&fil); /* close flushes to the card */

    if (fr == FR_OK && bw != (UINT)n) fr = FR_DISK_ERR;
    return (fr != FR_OK) ? fr : fc;
}

FRESULT sdlog(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FRESULT fr = log_va_list(fmt, ap);
    va_end(ap);
    return fr;
}

FRESULT sdlog_str(const char *s)
{
    return sdlog("%s", s);          /* "%s" keeps '%' in s safe */
}

/*
 * debug.c
 *
 *  Created on: 30 Oct 2026
 *      Author: matpj
 *
 *  Works on top of FatFs
 *  prints error description of corresponding function-return-error-codes
 */

#include <FatFs_debug.h>
#include "main.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>

static const char *const fr_text[] = { // array of pointers to strings
    "Function succeeded",                          /* 0  FR_OK */
    "Hard error in low level disk I/O layer",      /* 1  FR_DISK_ERR */
    "Assertion failed",                            /* 2  FR_INT_ERR */
    "Physical drive does not work",                /* 3  FR_NOT_READY */
    "Could not find the file",                     /* 4  FR_NO_FILE */
    "Could not find the path",                     /* 5  FR_NO_PATH */
    "Invalid path name format",                    /* 6  FR_INVALID_NAME */
    "Access denied or directory full",             /* 7  FR_DENIED */
    "Access denied (prohibited access)",           /* 8  FR_EXIST */
    "File/directory object is invalid",            /* 9  FR_INVALID_OBJECT */
    "Physical drive is write protected",           /* 10 FR_WRITE_PROTECTED */
    "Logical drive number is invalid",             /* 11 FR_INVALID_DRIVE */
    "Volume has no work area",                     /* 12 FR_NOT_ENABLED */
    "No valid FAT volume found",                   /* 13 FR_NO_FILESYSTEM */
    "f_mkfs aborted",                              /* 14 FR_MKFS_ABORTED */
    "Could not get control of the volume in time", /* 15 FR_TIMEOUT */
    "Rejected by file sharing policy",             /* 16 FR_LOCKED */
    "LFN buffer too small or path too deep",       /* 17 FR_NOT_ENOUGH_CORE */
    "Too many open files",                         /* 18 FR_TOO_MANY_OPEN_FILES */
    "Invalid parameter"                            /* 19 FR_INVALID_PARAMETER */
};


//this works with above 'return status' string pointers
const char *fr_str (FRESULT fr)
{
    if ((unsigned)fr < sizeof fr_text / sizeof fr_text[0])
        return fr_text[fr];
    return "Unknown error";
}


void uart_print(const char *fmt, ...) // variadic function
{
    char m[96];
    va_list ap;
    va_start(ap, fmt);
    int k = vsnprintf(m, sizeof m, fmt, ap);
    va_end(ap);
    if (k > (int)sizeof m - 1)
    	k = sizeof m - 1;
    if (k > 0)
    	HAL_UART_Transmit(&huart2, (uint8_t *)m, k, 100);
}



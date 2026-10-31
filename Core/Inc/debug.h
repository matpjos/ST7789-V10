/*
 * debug.h
 *
 *  Created on: 30 Oct 2026
 *      Author: matpj
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_
#include "ff.h"

/* printf-style output over UART2. The attribute makes the compiler check
 * that your %d / %s / %lu match the arguments you pass. */
void uart_print(const char *fmt, ...) __attribute__((format(printf, 1, 2))); //to-do revise this...

/* FRESULT code -> readable text */
const char *fr_str(FRESULT fr);

#endif /* INC_DEBUG_H_ */

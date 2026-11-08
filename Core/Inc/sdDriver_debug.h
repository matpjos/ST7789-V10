/*
 * sdDriver_debug.h
 *
 *  Created on: 24 Oct 2026
 *      Author: matpj
 */

#ifndef INC_SD_DEBUG_H_
#define INC_SD_DEBUG_H_
#include <stdint.h>

const char *sd_err_str(uint8_t rc);
void sd_dump_sector(uint32_t sector);
void sd_probe_partition(void);
void uart_hexdump(const uint8_t *buf, uint16_t len);


#endif /* INC_SDDRIVER_DEBUG_H_ */

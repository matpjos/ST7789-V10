/*
 * sd.h
 *
 *  Created on: 24 Oct 2026
 *      Author: matpj
 */

#ifndef INC_SD_H_
#define INC_SD_H_
#include <stdint.h>

uint8_t  sd_init(void);
uint8_t sd_read_block (uint32_t sector, uint8_t *buf);
uint8_t sd_write_block(uint32_t sector, const uint8_t *buf);

enum { 	SD_OK 				= 0,
		SD_ERR_CMD0 		= 1,
		SD_ERR_CMD8 		= 2,
		SD_ERR_ACMD41 		= 3,
		SD_ERR_CMD58 		= 4,
		SD_ERR_CMD16 		= 5,
		SD_ERR_CMD17	  	= 6,
		SD_ERR_READ_TOKEN	= 7,
		SD_ERR_CMD24  		= 8,
		SD_ERR_WRITE_DATA   = 9,
		SD_ERR_WRITE_BUSY   =10
	};

#endif /* INC_SD_H_ */

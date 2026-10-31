/*
 * tft.h
 *
 *  Created on: 24 Oct 2026
 *      Author: matpj
 */

#ifndef INC_TFT_H_
#define INC_TFT_H_

#include <stdint.h>

#define TFT_W 320
#define TFT_H 240

void tft_init(void);
void tft_fill(uint16_t colour);

void tft_draw_string(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg, uint8_t scale);


#endif /* INC_TFT_H_ */

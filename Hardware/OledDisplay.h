#ifndef HARDWARE_OLED_DISPLAY_H
#define HARDWARE_OLED_DISPLAY_H

#include <stdint.h>

void OledDisplay_Init(void);
void OledDisplay_Clear(void);
void OledDisplay_WriteLine(uint8_t line, const char *text);
void OledDisplay_WriteChar(uint8_t line, uint8_t char_index, char ch);
void OledDisplay_UpdateRegion(uint8_t page, uint8_t column_start, uint8_t column_end);
void OledDisplay_UpdateGlyph(uint8_t line, uint8_t char_index);
void OledDisplay_Update(void);

#endif

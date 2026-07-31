#ifndef HARDWARE_OLED_DISPLAY_H
#define HARDWARE_OLED_DISPLAY_H

#include <stdint.h>

void OledDisplay_Init(void);
void OledDisplay_Clear(void);
void OledDisplay_WriteLine(uint8_t line, const char *text);
void OledDisplay_Update(void);

#endif

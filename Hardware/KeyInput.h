#ifndef HARDWARE_KEY_INPUT_H
#define HARDWARE_KEY_INPUT_H

#include <stdint.h>

#define KEY_INPUT_K1_PRESSED 0x01U
#define KEY_INPUT_K2_PRESSED 0x02U

void KeyInput_Init(void);
uint8_t KeyInput_Update(uint32_t now_ms);

#endif

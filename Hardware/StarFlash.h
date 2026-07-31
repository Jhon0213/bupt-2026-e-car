#ifndef HARDWARE_STARFLASH_H
#define HARDWARE_STARFLASH_H

#include <stdbool.h>
#include <stdint.h>

/* H63 StarFlash transparent link on UART2: PA21 = TX, PA22 = RX, 115200 baud, 8-N-1. */
void StarFlash_Init(void);
bool StarFlash_ReadByte(uint8_t *byte);
void StarFlash_SendByte(uint8_t byte);
void StarFlash_SendString(const char *text);
uint32_t StarFlash_GetReceivedCount(void);
uint32_t StarFlash_GetOverflowCount(void);

#endif

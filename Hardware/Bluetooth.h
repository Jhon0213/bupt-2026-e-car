#ifndef HARDWARE_BLUETOOTH_H
#define HARDWARE_BLUETOOTH_H

#include <stdbool.h>
#include <stdint.h>

/* HC-06D on UART2: PA21 = TX, PA22 = RX, 9600 baud, 8-N-1. */
void Bluetooth_Init(void);
bool Bluetooth_ReadByte(uint8_t *byte);
void Bluetooth_SendByte(uint8_t byte);
void Bluetooth_SendString(const char *text);
uint32_t Bluetooth_GetReceivedCount(void);
uint32_t Bluetooth_GetOverflowCount(void);

#endif

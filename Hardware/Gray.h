#ifndef HARDWARE_GRAY_H_
#define HARDWARE_GRAY_H_

#include <stdint.h>

void Gray_Init(void);
void Gray_Update(void);
uint8_t Gray_GetRaw(void);
uint8_t Gray_GetDigital(void);
uint8_t Gray_GetBlackMask_ActiveLow(void);
uint8_t Gray_GetBlackMask_ActiveHigh(void);
uint8_t Gray_GetBlackMask(void);
int16_t Gray_GetError(void);
uint8_t Gray_IsLost(void);
uint8_t Gray_ReadDATRaw(void);
void Gray_DebugClockPulse(uint16_t count);
uint8_t Gray_DebugReadDAT8Times(void);

#endif /* HARDWARE_GRAY_H_ */

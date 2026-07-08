#ifndef HARDWARE_BUZZER_H_
#define HARDWARE_BUZZER_H_

#include <stdint.h>

/* Active buzzer module: PA13, high-level trigger. */
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
uint8_t Buzzer_IsOn(void);

#endif

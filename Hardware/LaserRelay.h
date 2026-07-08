#ifndef HARDWARE_LASER_RELAY_H_
#define HARDWARE_LASER_RELAY_H_

#include <stdint.h>

/* High-level-trigger relay input connected to PA12. */
void LaserRelay_Init(void);
void LaserRelay_On(void);
void LaserRelay_Off(void);
uint8_t LaserRelay_IsOn(void);

#endif

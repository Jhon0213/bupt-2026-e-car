#ifndef APPLICATION_STATUS_SIGNAL_H_
#define APPLICATION_STATUS_SIGNAL_H_

#include <stdint.h>

void StatusSignal_Init(void);
void StatusSignal_RequestKeyPoint(void);
void StatusSignal_RequestFinish(void);
void StatusSignal_RequestError(void);
void StatusSignal_Update(uint16_t elapsed_ms);
uint8_t StatusSignal_IsBusy(void);

#endif

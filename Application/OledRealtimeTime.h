#ifndef APPLICATION_OLED_REALTIME_TIME_H_
#define APPLICATION_OLED_REALTIME_TIME_H_

#include <stdint.h>

void OledRealtimeTime_Init(uint32_t start_ms);
void OledRealtimeTime_Request(uint32_t now_ms);
void OledRealtimeTime_ProcessOneStep(void);
void OledRealtimeTime_ForceFinal(uint32_t elapsed_ms);
void OledRealtimeTime_Reset(void);

#endif
#ifndef COMMUNICATION_INTERBOARD_UART_H_
#define COMMUNICATION_INTERBOARD_UART_H_

#include <stdint.h>

#include "Communication/InterboardProtocol.h"

void InterboardUart_Init(void);
void InterboardUart_SubmitLatest(const uint8_t frame[INTERBOARD_FRAME_SIZE]);
uint32_t InterboardUart_GetTxFrameCount(void);
uint32_t InterboardUart_GetOverwriteCount(void);
uint8_t InterboardUart_IsBusy(void);

#endif
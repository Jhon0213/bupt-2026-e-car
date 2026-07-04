#ifndef FLASH_H
#define FLASH_H

#include "ti_msp_dl_config.h"

/* Flash�洢���� */
#define FLASH_WHITE_DATA_ADDR  (0x0000F000) // ƫ�Ƶ�ַ
#define FLASH_BLACK_DATA_ADDR  (0x0000F010) // ƫ�Ƶ�ַ
/* �������� */
void writeToFlash(void);
void readWhiteFromFlash(void);
void readBlackFromFlash(void);
#endif // ADC_H

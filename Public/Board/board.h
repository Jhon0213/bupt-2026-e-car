
#ifndef __BOARD_H__
#define __BOARD_H__

#include "ti_msp_dl_config.h"
#include <stdio.h>

#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif

#ifndef u64
#define u64 uint64_t
#endif

void board_init(void);
void uart0_init(void);
void uart0_send_char(char ch);
void uart0_send_string(char* str);
int fputc(int ch, FILE *f);
/* 延时函数 */
void delay_us(int __us);
void delay_ms(int __ms);

void delay_1us(int __us);
void delay_1ms(int __ms);


struct SAngle
{
		float Yaw;
};
extern struct SAngle stcAngle;

struct SGyro
{
    short rawWz;
    float wz;
};
extern struct SGyro stcGyro;
float GyroZ(void);
float Yaw(void);
void sendCaliYawCommand(void);
void performCaliBias(void);
	
#endif

/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "board.h"
#include "lcd_init.h"
#include "lcd.h"
#include "Motor.h"
#include "Encoder.h"
#include "control.h"
#include "stdio.h"

int main(void)
{
    board_init();
		LCD_Init();//LCD≥ı ºªØ
    LCD_Fill(0,0,LCD_W,LCD_H,BLACK);
		sendCaliYawCommand();
		Motor_Init();
		Encoder_Init();
//		performCaliBias();
		while(1)
		{
			printf("%.4f,%.4f\r\n",GyroZ(),Yaw());
//			delay_ms(100);
//			LCD_ShowFloatNumEx(0, 0, GyroZ(), 4, WHITE, BLACK, 16);
//			LCD_ShowFloatNumEx(0, 20, Yaw(), 4, WHITE, BLACK, 16);
//			LCD_ShowIntNum(0,40,motor_1.countnum,WHITE, BLACK, 16);
//			LCD_ShowIntNum(0,60,motor_2.countnum,WHITE, BLACK, 16);
//			LCD_ShowIntNum(60,40,motor_1.speed,WHITE, BLACK, 16);
//			LCD_ShowIntNum(60,60,motor_2.speed,WHITE, BLACK, 16);
//      yaw_control_with_gyro(200, 0);    
//			Speed_Control(0, 0);
		}
		
}

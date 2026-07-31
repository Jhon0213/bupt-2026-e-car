#include "Application/Task1_AutoTrace.h"

#include "Hardware/Motor.h"
#include "Hardware/StarFlash.h"
#include "Public/Board/board.h"

void Task1_AutoTrace_Run(void)
{
    StarFlash_SendString("TASK1_STANDBY,MOTOR_BRAKE\r\n");
    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}
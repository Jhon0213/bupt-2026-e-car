#include "Application/RobotPlatform.h"
#include "Application/Task1_AutoTrace.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

int main(void)
{
    RobotPlatform_Init();

    /* Future key/serial mode selection belongs here. */
    Task1_AutoTrace_Run();

    while (1)
    {
        Motor_Coast();
        delay_ms(100U);
    }
}

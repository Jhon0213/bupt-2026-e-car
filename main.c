#include "Application/RobotPlatform.h"
#include "Application/Task1_AutoTrace.h"
#include "Application/Task3_LinkedOperation.h"
#include "Application/TaskBonus1_LaserTrace.h"
#include "Application/TaskBonusFourLap.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define TASK_MODE_TASK1_AUTO_TRACE       1U
#define TASK_MODE_TASK3_LINKED_OPERATION 3U
#define TASK_MODE_BONUS_FOUR_LAP         4U
#define TASK_MODE_BONUS1_LASER_TRACE     5U

/* Change only this value to select the program started after power-on. */
#define SELECTED_TASK_MODE  TASK_MODE_BONUS1_LASER_TRACE

int main(void)
{
    RobotPlatform_Init();
    LaserRelay_Off();
    Motor_Brake();
    delay_ms(2000U);

#if SELECTED_TASK_MODE == TASK_MODE_TASK1_AUTO_TRACE
    Task1_AutoTrace_Run();

#elif SELECTED_TASK_MODE == TASK_MODE_TASK3_LINKED_OPERATION
    Task3_LinkedOperation_Run();

#elif SELECTED_TASK_MODE == TASK_MODE_BONUS_FOUR_LAP
    TaskBonusFourLap_Start();

    while (TaskBonusFourLap_IsFinished() == 0U)
    {
        delay_ms(TASK_BONUS_FOUR_LAP_UPDATE_MS);
        TaskBonusFourLap_Update();
    }

#elif SELECTED_TASK_MODE == TASK_MODE_BONUS1_LASER_TRACE
    TaskBonus1_LaserTrace_Run();

#else
#error "SELECTED_TASK_MODE is not implemented"
#endif

    while (1)
    {
        Motor_Coast();
        delay_ms(100U);
    }
}

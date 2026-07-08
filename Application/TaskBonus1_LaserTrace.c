#include <stdint.h>

#include "Application/TaskBonus1_LaserTrace.h"

#include "Application/RouteNavigator.h"
#include "Application/StatusSignal.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define TASK_BONUS1_TOTAL_TIMEOUT_MS  40000U
#define TASK_BONUS1_LASER_SETTLE_MS     500U
#define TASK_BONUS1_FINAL_BRAKE_MS      300U

void TaskBonus1_LaserTrace_Run(void)
{
    uint32_t total_ms = 0U;
    uint32_t stopped_ms = 0U;
    uint32_t events;
    uint8_t error_signaled = 0U;

    LaserRelay_Off();
    LaserRelay_On();
    delay_ms(TASK_BONUS1_LASER_SETTLE_MS);
    RouteNavigator_Start();

    while ((RouteNavigator_IsFinished() == 0U) &&
           (total_ms < TASK_BONUS1_TOTAL_TIMEOUT_MS))
    {
        delay_ms(ROUTE_NAVIGATOR_CONTROL_MS);
        total_ms += ROUTE_NAVIGATOR_CONTROL_MS;

        RouteNavigator_Update();
        events = RouteNavigator_ConsumeEvents();

        if ((events & ROUTE_EVENT_ERROR) != 0U)
        {
            LaserRelay_Off();
            StatusSignal_RequestError();
            error_signaled = 1U;
        }
        else if ((events & ROUTE_EVENT_LAP_COMPLETE) != 0U)
        {
            LaserRelay_Off();
            StatusSignal_RequestFinish();
        }

        StatusSignal_Update(ROUTE_NAVIGATOR_CONTROL_MS);
    }

    LaserRelay_Off();

    if ((total_ms >= TASK_BONUS1_TOTAL_TIMEOUT_MS) &&
        (RouteNavigator_IsFinished() == 0U))
    {
        RouteNavigator_Abort();
        (void)RouteNavigator_ConsumeEvents();
        StatusSignal_RequestError();
        error_signaled = 1U;
    }
    else if ((RouteNavigator_GetResult() != ROUTE_RESULT_LAP_COMPLETE) &&
             (error_signaled == 0U))
    {
        StatusSignal_RequestError();
    }

    while ((stopped_ms < TASK_BONUS1_FINAL_BRAKE_MS) ||
           (StatusSignal_IsBusy() != 0U))
    {
        LaserRelay_Off();
        Motor_Brake();
        delay_ms(ROUTE_NAVIGATOR_CONTROL_MS);
        StatusSignal_Update(ROUTE_NAVIGATOR_CONTROL_MS);
        stopped_ms += ROUTE_NAVIGATOR_CONTROL_MS;
    }

    LaserRelay_Off();
    Motor_Coast();
    RouteNavigator_SendLogs();
}

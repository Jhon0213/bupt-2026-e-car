#include <stdint.h>

#include "Application/Task3_LinkedOperation.h"

#include "Application/RouteNavigator.h"
#include "Application/StatusSignal.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define TASK3_AIM_PLACEHOLDER_MS   1000U
#define TASK3_LASER_ON_MS          1000U
#define TASK3_TOTAL_TIMEOUT_MS     40000U
#define TASK3_FINAL_BRAKE_MS         300U

typedef enum
{
    TASK3_STATE_ROUTE = 0,
    TASK3_STATE_AIM_WAIT,
    TASK3_STATE_LASER_ON,
    TASK3_STATE_FINISHED
} Task3_State;

void Task3_LinkedOperation_Run(void)
{
    Task3_State task_state = TASK3_STATE_ROUTE;
    uint32_t total_ms = 0U;
    uint32_t aim_ms = 0U;
    uint32_t laser_ms = 0U;
    uint32_t stopped_ms = 0U;
    uint32_t events;
    uint8_t b_handled = 0U;

    LaserRelay_Off();
    RouteNavigator_Start();

    while ((task_state != TASK3_STATE_FINISHED) &&
           (total_ms < TASK3_TOTAL_TIMEOUT_MS))
    {
        delay_ms(ROUTE_NAVIGATOR_CONTROL_MS);
        total_ms += ROUTE_NAVIGATOR_CONTROL_MS;

        RouteNavigator_Update();
        events = RouteNavigator_ConsumeEvents();

        if ((events & ROUTE_EVENT_ERROR) != 0U)
        {
            LaserRelay_Off();
            StatusSignal_RequestError();
            task_state = TASK3_STATE_FINISHED;
        }
        else if (((events & ROUTE_EVENT_REACHED_B) != 0U) &&
                 (b_handled == 0U))
        {
            b_handled = 1U;
            aim_ms = 0U;
            laser_ms = 0U;
            LaserRelay_Off();
            RouteNavigator_Pause();
            Motor_Brake();
            StatusSignal_RequestKeyPoint();
            task_state = TASK3_STATE_AIM_WAIT;
        }
        else if ((events & ROUTE_EVENT_LAP_COMPLETE) != 0U)
        {
            LaserRelay_Off();
            StatusSignal_RequestFinish();
            task_state = TASK3_STATE_FINISHED;
        }
        else if ((events & (ROUTE_EVENT_REACHED_C |
                            ROUTE_EVENT_REACHED_D)) != 0U)
        {
            StatusSignal_RequestKeyPoint();
        }

        if (task_state == TASK3_STATE_AIM_WAIT)
        {
            Motor_Brake();
            aim_ms += ROUTE_NAVIGATOR_CONTROL_MS;

            if (aim_ms >= TASK3_AIM_PLACEHOLDER_MS)
            {
                laser_ms = 0U;
                LaserRelay_On();
                StatusSignal_RequestKeyPoint();
                task_state = TASK3_STATE_LASER_ON;
            }
        }
        else if (task_state == TASK3_STATE_LASER_ON)
        {
            Motor_Brake();
            laser_ms += ROUTE_NAVIGATOR_CONTROL_MS;

            if (laser_ms >= TASK3_LASER_ON_MS)
            {
                LaserRelay_Off();
                RouteNavigator_Resume();
                task_state = TASK3_STATE_ROUTE;
            }
        }

        StatusSignal_Update(ROUTE_NAVIGATOR_CONTROL_MS);

        if (RouteNavigator_IsFinished() != 0U)
            task_state = TASK3_STATE_FINISHED;
    }

    LaserRelay_Off();

    if ((total_ms >= TASK3_TOTAL_TIMEOUT_MS) &&
        (RouteNavigator_IsFinished() == 0U))
    {
        RouteNavigator_Abort();
        (void)RouteNavigator_ConsumeEvents();
        StatusSignal_RequestError();
    }

    while ((stopped_ms < TASK3_FINAL_BRAKE_MS) ||
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

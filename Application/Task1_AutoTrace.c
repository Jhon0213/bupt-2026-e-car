#include "Application/Task1_AutoTrace.h"

#include "Application/RouteNavigator.h"
#include "Application/StatusSignal.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define TASK1_FINAL_BRAKE_MS  300U

void Task1_AutoTrace_Run(void)
{
    uint32_t events;
    uint32_t stopped_ms = 0U;

    RouteNavigator_Start();

    while (RouteNavigator_IsFinished() == 0U)
    {
        delay_ms(ROUTE_NAVIGATOR_CONTROL_MS);
        RouteNavigator_Update();
        events = RouteNavigator_ConsumeEvents();

        if ((events & ROUTE_EVENT_LAP_COMPLETE) != 0U)
            StatusSignal_RequestFinish();
        else if ((events & ROUTE_EVENT_ERROR) != 0U)
            StatusSignal_RequestError();

        StatusSignal_Update(ROUTE_NAVIGATOR_CONTROL_MS);
    }

    while ((stopped_ms < TASK1_FINAL_BRAKE_MS) ||
           (StatusSignal_IsBusy() != 0U))
    {
        Motor_Brake();
        delay_ms(ROUTE_NAVIGATOR_CONTROL_MS);
        StatusSignal_Update(ROUTE_NAVIGATOR_CONTROL_MS);
        stopped_ms += ROUTE_NAVIGATOR_CONTROL_MS;
    }

    Motor_Coast();
    RouteNavigator_SendLogs();
}
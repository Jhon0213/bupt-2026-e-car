#include "Application/TaskBonusFourLap.h"

#include "Application/RouteNavigator.h"
#include "Application/StatusSignal.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"

#define TASK_BONUS_FOUR_LAP_COUNT           4U
#define TASK_BONUS_FOUR_LAP_FINAL_BRAKE_MS 300U

typedef enum
{
    TASK_BONUS_FOUR_LAP_STATE_IDLE = 0,
    TASK_BONUS_FOUR_LAP_STATE_ROUTE,
    TASK_BONUS_FOUR_LAP_STATE_FINAL_BRAKE,
    TASK_BONUS_FOUR_LAP_STATE_FINISHED
} TaskBonusFourLap_State;

static TaskBonusFourLap_State task_state;
static TaskBonusFourLap_Result task_result;
static uint32_t stopped_ms;
static uint8_t current_lap;

static void StartCurrentLap(void)
{
    RouteNavigator_SetContinuationRequired(
        (current_lap < TASK_BONUS_FOUR_LAP_COUNT) ? 1U : 0U);
    RouteNavigator_LogSessionSelectLap(
        current_lap,
        (current_lap == TASK_BONUS_FOUR_LAP_COUNT) ? 1U : 0U);
    RouteNavigator_Start();
}

static void BeginFinalBrake(TaskBonusFourLap_Result result)
{
    LaserRelay_Off();
    task_result = result;
    stopped_ms = 0U;
    Motor_Brake();
    task_state = TASK_BONUS_FOUR_LAP_STATE_FINAL_BRAKE;
}

void TaskBonusFourLap_Start(void)
{
    if ((task_state == TASK_BONUS_FOUR_LAP_STATE_ROUTE) ||
        (task_state == TASK_BONUS_FOUR_LAP_STATE_FINAL_BRAKE))
    {
        return;
    }

    current_lap = 1U;
    stopped_ms = 0U;
    task_result = TASK_BONUS_FOUR_LAP_RESULT_NONE;

    LaserRelay_Off();
    LaserRelay_On();
    RouteNavigator_LogSessionBegin();
    StartCurrentLap();
    task_state = TASK_BONUS_FOUR_LAP_STATE_ROUTE;
}

void TaskBonusFourLap_Update(void)
{
    uint32_t events;

    if (task_state == TASK_BONUS_FOUR_LAP_STATE_ROUTE)
    {
        RouteNavigator_Update();
        events = RouteNavigator_ConsumeEvents();

        if ((events & ROUTE_EVENT_ERROR) != 0U)
        {
            LaserRelay_Off();
            StatusSignal_RequestError();
        }
        else if ((events & ROUTE_EVENT_LAP_COMPLETE) != 0U)
        {
            if (current_lap == TASK_BONUS_FOUR_LAP_COUNT)
                StatusSignal_RequestFinish();
            else
                StatusSignal_RequestKeyPoint();
        }

        StatusSignal_Update(TASK_BONUS_FOUR_LAP_UPDATE_MS);

        if (RouteNavigator_IsFinished() != 0U)
        {
            if (RouteNavigator_GetResult() != ROUTE_RESULT_LAP_COMPLETE)
            {
                if ((events & ROUTE_EVENT_ERROR) == 0U)
                    StatusSignal_RequestError();

                BeginFinalBrake(TASK_BONUS_FOUR_LAP_RESULT_ROUTE_ERROR);
            }
            else if (current_lap < TASK_BONUS_FOUR_LAP_COUNT)
            {
                current_lap++;
                StartCurrentLap();
            }
            else
            {
                BeginFinalBrake(TASK_BONUS_FOUR_LAP_RESULT_COMPLETE);
            }
        }
    }
    else if (task_state == TASK_BONUS_FOUR_LAP_STATE_FINAL_BRAKE)
    {
        LaserRelay_Off();
        Motor_Brake();
        StatusSignal_Update(TASK_BONUS_FOUR_LAP_UPDATE_MS);
        stopped_ms += TASK_BONUS_FOUR_LAP_UPDATE_MS;

        if ((stopped_ms >= TASK_BONUS_FOUR_LAP_FINAL_BRAKE_MS) &&
            (StatusSignal_IsBusy() == 0U))
        {
            LaserRelay_Off();
            Motor_Coast();
            RouteNavigator_SendLogs();
            task_state = TASK_BONUS_FOUR_LAP_STATE_FINISHED;
        }
    }
}

void TaskBonusFourLap_Abort(void)
{
    if (task_state == TASK_BONUS_FOUR_LAP_STATE_ROUTE)
    {
        RouteNavigator_Abort();
        (void)RouteNavigator_ConsumeEvents();
        StatusSignal_RequestError();
        BeginFinalBrake(TASK_BONUS_FOUR_LAP_RESULT_ABORTED);
    }
}

uint8_t TaskBonusFourLap_IsRunning(void)
{
    return ((task_state == TASK_BONUS_FOUR_LAP_STATE_ROUTE) ||
            (task_state == TASK_BONUS_FOUR_LAP_STATE_FINAL_BRAKE)) ? 1U : 0U;
}

uint8_t TaskBonusFourLap_IsFinished(void)
{
    return (task_state == TASK_BONUS_FOUR_LAP_STATE_FINISHED) ? 1U : 0U;
}

uint8_t TaskBonusFourLap_GetLap(void)
{
    return current_lap;
}

TaskBonusFourLap_Result TaskBonusFourLap_GetResult(void)
{
    return task_result;
}

#ifndef APPLICATION_ROUTE_NAVIGATOR_H_
#define APPLICATION_ROUTE_NAVIGATOR_H_

#include <stdint.h>

#define ROUTE_NAVIGATOR_CONTROL_MS 10U

typedef enum
{
    ROUTE_STATE_IDLE = 0,
    ROUTE_STATE_HEADING_AB = 1,
    ROUTE_STATE_CAPTURE_B = 2,
    ROUTE_STATE_GRAY_BC = 3,
    ROUTE_STATE_ADVANCE_C = 4,
    ROUTE_STATE_ALIGN_C = 5,
    ROUTE_STATE_HEADING_CD = 6,
    ROUTE_STATE_CAPTURE_D = 7,
    ROUTE_STATE_GRAY_DA = 8,
    ROUTE_STATE_ADVANCE_A = 9,
    ROUTE_STATE_BRAKE_C = 10,
    ROUTE_STATE_WAIT_IMU = 11,
    ROUTE_STATE_FINISHED = 12,
    ROUTE_STATE_BRAKE_A = 13,
    ROUTE_STATE_ALIGN_A = 14
} RouteNavigator_State;

typedef enum
{
    ROUTE_RESULT_NONE = 0,
    ROUTE_RESULT_LAP_COMPLETE = 1,
    ROUTE_RESULT_EARLY_LOST = 2,
    ROUTE_RESULT_TIMEOUT = 3,
    ROUTE_RESULT_NO_IMU = 4,
    ROUTE_RESULT_CAPTURE_TIMEOUT = 5,
    ROUTE_RESULT_ABORTED = 6,
    ROUTE_RESULT_ALIGN_TIMEOUT = 7
} RouteNavigator_Result;

typedef enum
{
    ROUTE_EVENT_NONE = 0U,
    ROUTE_EVENT_REACHED_B = 1U << 0,
    ROUTE_EVENT_REACHED_C = 1U << 1,
    ROUTE_EVENT_REACHED_D = 1U << 2,
    ROUTE_EVENT_REACHED_A = 1U << 3,
    ROUTE_EVENT_LAP_COMPLETE = 1U << 4,
    ROUTE_EVENT_ERROR = 1U << 5
} RouteNavigator_Event;

/* Call once every 10 ms. The function never delays or blocks. */
void RouteNavigator_Start(void);
void RouteNavigator_Update(void);
void RouteNavigator_Pause(void);
void RouteNavigator_Resume(void);
void RouteNavigator_Abort(void);
void RouteNavigator_SetContinuationRequired(uint8_t required);

/* Optional buffered diagnostics across multiple laps. */
void RouteNavigator_LogSessionBegin(void);
void RouteNavigator_LogSessionSelectLap(uint8_t lap, uint8_t detailed);

uint8_t RouteNavigator_IsRunning(void);
uint8_t RouteNavigator_IsPaused(void);
uint8_t RouteNavigator_IsFinished(void);
RouteNavigator_State RouteNavigator_GetState(void);
RouteNavigator_Result RouteNavigator_GetResult(void);
uint32_t RouteNavigator_ConsumeEvents(void);

/* Send buffered diagnostic data after a run has ended. */
void RouteNavigator_SendLogs(void);

#endif

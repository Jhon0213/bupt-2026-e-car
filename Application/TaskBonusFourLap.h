#ifndef APPLICATION_TASK_BONUS_FOUR_LAP_H_
#define APPLICATION_TASK_BONUS_FOUR_LAP_H_

#include <stdint.h>

#define TASK_BONUS_FOUR_LAP_UPDATE_MS  10U

typedef enum
{
    TASK_BONUS_FOUR_LAP_RESULT_NONE = 0,
    TASK_BONUS_FOUR_LAP_RESULT_COMPLETE,
    TASK_BONUS_FOUR_LAP_RESULT_ROUTE_ERROR,
    TASK_BONUS_FOUR_LAP_RESULT_ABORTED
} TaskBonusFourLap_Result;

/* Start one four-lap session. Call Update once every 10 ms until finished. */
void TaskBonusFourLap_Start(void);
void TaskBonusFourLap_Update(void);
void TaskBonusFourLap_Abort(void);

uint8_t TaskBonusFourLap_IsRunning(void);
uint8_t TaskBonusFourLap_IsFinished(void);
uint8_t TaskBonusFourLap_GetLap(void);
TaskBonusFourLap_Result TaskBonusFourLap_GetResult(void);

#endif

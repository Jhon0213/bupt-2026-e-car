#ifndef APPLICATION_TASK3_LINKED_OPERATION_H_
#define APPLICATION_TASK3_LINKED_OPERATION_H_

#include <stdint.h>

/* Task 3: six-channel gray line tracking using the software I2C sensor. */
typedef enum
{
    TASK3_RUN_ONE_LAP = 0,
    TASK3_RUN_B_PLUS_5CM,
    TASK3_RUN_ONE_LAP_ALT
} Task3_RunMode;
void Task3_LinkedOperation_Run(void);
void Task3_LinkedOperation_Start(uint32_t now_ms);
void Task3_LinkedOperation_StartMode(uint32_t now_ms, Task3_RunMode mode);
void Task3_LinkedOperation_SetDebugEnabled(uint8_t enabled);
void Task3_LinkedOperation_Stop(void);
void Task3_LinkedOperation_Update(uint32_t now_ms);
uint8_t Task3_LinkedOperation_IsRunning(void);
const char *Task3_LinkedOperation_GetSegmentText(void);
int16_t Task3_LinkedOperation_GetProgressX10(void);
int32_t Task3_LinkedOperation_GetOdometerCount(void);

#endif


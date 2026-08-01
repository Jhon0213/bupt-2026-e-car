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

typedef enum
{
    TASK3_STOP_REASON_NONE = 0U,
    TASK3_STOP_REASON_USER,
    TASK3_STOP_REASON_FINISHED,
    TASK3_STOP_REASON_LINE_LOST,
    TASK3_STOP_REASON_GRAY_FAULT,
    TASK3_STOP_REASON_INTERNAL_FAULT
} Task3_StopReason;

typedef struct
{
    uint8_t running;
    uint8_t turning;
    uint8_t line_valid;
    uint8_t finished;
    Task3_StopReason stop_reason;
} Task3_ApplicationState;

typedef struct
{
    uint8_t segment;
    uint8_t control_phase;
    int32_t progress_x10;
    int16_t gray_error;
    int16_t raw_correction_rpm_x10;
    int16_t applied_correction_rpm_x10;
    int16_t base_rpm_x10;
    int16_t left_target_rpm_x10;
    int16_t right_target_rpm_x10;
    int16_t left_actual_rpm_x10;
    int16_t right_actual_rpm_x10;
    int16_t left_pwm;
    int16_t right_pwm;
    int16_t left_integral_x10;
    int16_t right_integral_x10;
    int32_t left_encoder_delta;
    int32_t right_encoder_delta;
    int32_t left_minus_right_count;
    uint8_t black_mask;
    uint8_t line_lost;
    uint8_t curve_lost_hold;
    uint8_t straight_slew_active;
} Task3_DiagSnapshot;

void Task3_LinkedOperation_Run(void);
void Task3_LinkedOperation_Start(uint32_t now_ms);
void Task3_LinkedOperation_StartMode(uint32_t now_ms, Task3_RunMode mode);
void Task3_LinkedOperation_SetDebugEnabled(uint8_t enabled);
void Task3_LinkedOperation_Stop(void);
void Task3_LinkedOperation_StopByUser(void);
void Task3_LinkedOperation_Update(uint32_t now_ms);
uint8_t Task3_LinkedOperation_IsRunning(void);
const char *Task3_LinkedOperation_GetSegmentText(void);
int16_t Task3_LinkedOperation_GetProgressX10(void);
int32_t Task3_LinkedOperation_GetOdometerCount(void);
const Task3_DiagSnapshot *Task3_LinkedOperation_GetDiagSnapshot(void);
void Task3_LinkedOperation_CopyDiagSnapshot(Task3_DiagSnapshot *snapshot);
void Task3_LinkedOperation_GetApplicationState(Task3_ApplicationState *state);

#endif
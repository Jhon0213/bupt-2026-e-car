#include "Application/TaskDualLoopDiag.h"
#include "Application/BuildConfig.h"

#include <stdio.h>

#include "Application/Task3_LinkedOperation.h"
#include "Hardware/Diagnostics/ControlTimingDiag.h"
#include "Hardware/Motor.h"
#include "Hardware/StarFlash.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Public/Board/board.h"

#ifndef DUAL_LOOP_DIAG_SELECTED_CASE
#define DUAL_LOOP_DIAG_SELECTED_CASE DUAL_LOOP_DIAG_ONE_LAP
#endif

#define DUAL_LOOP_DIAG_CONTROL_MS          10U
#define DUAL_LOOP_DIAG_VOFA_MS             50U
#define DUAL_LOOP_DIAG_STARTUP_IDLE_MS    500U
#define DUAL_LOOP_DIAG_STARTUP_STOP_MS   3500U
#define DUAL_LOOP_DIAG_STARTUP_END_MS    4200U
#define DUAL_LOOP_DIAG_STOP_FRAME_MS     1000U

static uint32_t g_dual_loop_late_count;
static uint32_t g_dual_loop_vofa_tx_count;

static DualLoopDiagCase TaskDualLoopDiag_GetSelectedCase(void)
{
    return (DualLoopDiagCase)DUAL_LOOP_DIAG_SELECTED_CASE;
}

static void TaskDualLoopDiag_SendBytes(const char *buffer, uint32_t length)
{
    uint32_t i;

    for (i = 0U; i < length; i++)
    {
        StarFlash_SendByte((uint8_t)buffer[i]);
    }
}

static void TaskDualLoopDiag_SendFrame(uint32_t elapsed_ms,
                                       DualLoopDiagCase diag_case,
                                       const Task3_DiagSnapshot *snapshot)
{
    char buffer[512];
    int length;
    int32_t left_minus_right_actual;
    uint32_t csv_start_ms = board_millis();

    left_minus_right_actual = (int32_t)snapshot->left_actual_rpm_x10 -
                              (int32_t)snapshot->right_actual_rpm_x10;

    length = snprintf(buffer,
                      sizeof(buffer),
                      "%lu,%lu,%lu,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%lu,%lu,%lu,%lu,%lu,%lu\r\n",
                      (unsigned long)elapsed_ms,
                      (unsigned long)diag_case,
                      (unsigned long)snapshot->segment,
                      (long)snapshot->progress_x10,
                      (long)snapshot->gray_error,
                      (long)snapshot->raw_correction_rpm_x10,
                      (long)snapshot->applied_correction_rpm_x10,
                      (long)snapshot->base_rpm_x10,
                      (long)snapshot->left_target_rpm_x10,
                      (long)snapshot->right_target_rpm_x10,
                      (long)snapshot->left_actual_rpm_x10,
                      (long)snapshot->right_actual_rpm_x10,
                      (long)left_minus_right_actual,
                      (long)snapshot->left_pwm,
                      (long)snapshot->right_pwm,
                      (long)snapshot->left_integral_x10,
                      (long)snapshot->right_integral_x10,
                      (long)snapshot->left_encoder_delta,
                      (long)snapshot->right_encoder_delta,
                      (long)snapshot->left_minus_right_count,
                      (unsigned long)snapshot->black_mask,
                      (unsigned long)snapshot->line_lost,
                      (unsigned long)snapshot->curve_lost_hold,
                      (unsigned long)snapshot->straight_slew_active,
                      (unsigned long)g_dual_loop_late_count,
                      (unsigned long)g_dual_loop_vofa_tx_count);
    if ((length <= 0) || ((uint32_t)length >= sizeof(buffer)))
    {
        return;
    }

    TaskDualLoopDiag_SendBytes(buffer, (uint32_t)length);
    g_dual_loop_vofa_tx_count++;
    ControlTimingDiag_RecordCsvDuration(board_millis() - csv_start_ms);
}

static void TaskDualLoopDiag_SendIfDue(uint32_t elapsed_ms,
                                       uint32_t *next_vofa_ms,
                                       DualLoopDiagCase diag_case)
{
#if !DUAL_LOOP_DIAG_TELEMETRY_ENABLE
    (void)elapsed_ms;
    (void)next_vofa_ms;
    (void)diag_case;
    return;
#else
    Task3_DiagSnapshot snapshot;

    if (elapsed_ms < *next_vofa_ms)
    {
        return;
    }

    Task3_LinkedOperation_CopyDiagSnapshot(&snapshot);
    TaskDualLoopDiag_SendFrame(elapsed_ms, diag_case, &snapshot);
    *next_vofa_ms += DUAL_LOOP_DIAG_VOFA_MS;
#endif
}
static void TaskDualLoopDiag_StopFormalTask(void)
{
    Task3_LinkedOperation_Stop();
    SpeedPI_Reset();
    Motor_Coast();
}

static void TaskDualLoopDiag_RecordLateIfNeeded(void)
{
    if (board_pending_control_ticks() != 0U)
    {
        g_dual_loop_late_count++;
    }
}

static void TaskDualLoopDiag_RunStartupCase(void)
{
    uint32_t start_ms;
    uint32_t next_vofa_ms = 0U;
    uint8_t stopped = 0U;

    ControlTimingDiag_Reset();
    g_dual_loop_late_count = 0U;
    g_dual_loop_vofa_tx_count = 0U;
    Task3_LinkedOperation_SetDebugEnabled(0U);
    TaskDualLoopDiag_StopFormalTask();
    board_clear_control_ticks();

    start_ms = board_millis();
    Task3_LinkedOperation_StartMode(start_ms + DUAL_LOOP_DIAG_STARTUP_IDLE_MS,
                                    TASK3_RUN_ONE_LAP);
    Task3_LinkedOperation_SetDebugEnabled(0U);

    while (1)
    {
        uint32_t now_ms = board_millis();
        uint32_t elapsed_ms = now_ms - start_ms;

        if (elapsed_ms >= DUAL_LOOP_DIAG_STARTUP_END_MS)
        {
            TaskDualLoopDiag_StopFormalTask();
            break;
        }

        if (board_consume_control_tick() != 0U)
        {
            now_ms = board_millis();
            elapsed_ms = now_ms - start_ms;

            if (elapsed_ms < DUAL_LOOP_DIAG_STARTUP_IDLE_MS)
            {
                SpeedPI_Reset();
                Motor_Coast();
            }
            else if (elapsed_ms < DUAL_LOOP_DIAG_STARTUP_STOP_MS)
            {
                Task3_LinkedOperation_Update(now_ms);
            }
            else
            {
                if (stopped == 0U)
                {
                    TaskDualLoopDiag_StopFormalTask();
                    stopped = 1U;
                }
                else
                {
                    SpeedPI_Reset();
                    Motor_Coast();
                }
            }

            TaskDualLoopDiag_RecordLateIfNeeded();
        }

        TaskDualLoopDiag_SendIfDue(elapsed_ms,
                                   &next_vofa_ms,
                                   DUAL_LOOP_DIAG_AB_STARTUP);
    }

    while (1)
    {
        TaskDualLoopDiag_StopFormalTask();
        delay_ms(100U);
    }
}

static void TaskDualLoopDiag_SendStoppedFrames(uint32_t start_ms,
                                               uint32_t *next_vofa_ms,
                                               DualLoopDiagCase diag_case)
{
    uint32_t stop_start_ms = board_millis();

    while ((board_millis() - stop_start_ms) < DUAL_LOOP_DIAG_STOP_FRAME_MS)
    {
        uint32_t elapsed_ms;

        TaskDualLoopDiag_StopFormalTask();
        elapsed_ms = board_millis() - start_ms;
        TaskDualLoopDiag_SendIfDue(elapsed_ms, next_vofa_ms, diag_case);
        delay_ms(5U);
    }
}

static void TaskDualLoopDiag_HoldStopped(void)
{
    while (1)
    {
        TaskDualLoopDiag_StopFormalTask();
        delay_ms(100U);
    }
}

static void TaskDualLoopDiag_RunOneLapCase(void)
{
    uint32_t start_ms;
    uint32_t next_vofa_ms = 0U;

    ControlTimingDiag_Reset();
    g_dual_loop_late_count = 0U;
    g_dual_loop_vofa_tx_count = 0U;
    Task3_LinkedOperation_SetDebugEnabled(0U);
    TaskDualLoopDiag_StopFormalTask();
    board_clear_control_ticks();

    start_ms = board_millis();
    Task3_LinkedOperation_StartMode(start_ms, TASK3_RUN_ONE_LAP);
    Task3_LinkedOperation_SetDebugEnabled(0U);

    while (Task3_LinkedOperation_IsRunning() != 0U)
    {
        uint32_t now_ms = board_millis();
        uint32_t elapsed_ms = now_ms - start_ms;

        if (board_consume_control_tick() != 0U)
        {
            now_ms = board_millis();
            elapsed_ms = now_ms - start_ms;
            Task3_LinkedOperation_Update(now_ms);
            TaskDualLoopDiag_RecordLateIfNeeded();
        }

        TaskDualLoopDiag_SendIfDue(elapsed_ms,
                                   &next_vofa_ms,
                                   DUAL_LOOP_DIAG_ONE_LAP);
    }

    TaskDualLoopDiag_StopFormalTask();
    TaskDualLoopDiag_SendStoppedFrames(start_ms,
                                       &next_vofa_ms,
                                       DUAL_LOOP_DIAG_ONE_LAP);
    TaskDualLoopDiag_HoldStopped();
}

void TaskDualLoopDiag_Run(void)
{
    DualLoopDiagCase selected_case = TaskDualLoopDiag_GetSelectedCase();

    switch (selected_case)
    {
        case DUAL_LOOP_DIAG_ONE_LAP:
            TaskDualLoopDiag_RunOneLapCase();
            return;

        case DUAL_LOOP_DIAG_AB_STARTUP:
            TaskDualLoopDiag_RunStartupCase();
            return;

        default:
            TaskDualLoopDiag_HoldStopped();
            return;
    }
}
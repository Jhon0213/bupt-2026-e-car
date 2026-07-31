#ifndef HARDWARE_DIAGNOSTICS_CONTROL_TIMING_DIAG_H_
#define HARDWARE_DIAGNOSTICS_CONTROL_TIMING_DIAG_H_

#include <stdint.h>

typedef struct
{
    uint32_t control_count;

    uint32_t period_sample_count;
    uint32_t period_min_ms;
    uint32_t period_max_ms;
    uint64_t period_sum_ms;

    uint32_t period_0ms_count;
    uint32_t period_10ms_count;
    uint32_t period_20ms_count;
    uint32_t period_30ms_count;
    uint32_t period_over_30ms_count;

    uint32_t exec_sample_count;
    uint32_t exec_max_ms;
    uint32_t exec_0ms_count;
    uint32_t exec_10ms_count;
    uint32_t exec_20ms_count;
    uint32_t exec_over_20ms_count;

    uint32_t catchup_event_count;
    uint32_t catchup_step_count;
    uint32_t backlog_drop_event_count;

    uint32_t csv_send_count;
    uint32_t csv_exec_max_ms;
    uint32_t csv_over_10ms_count;

    uint32_t oled_refresh_count;
    uint32_t oled_exec_max_ms;
    uint32_t oled_over_10ms_count;

    uint32_t oled_partial_write_count;
    uint32_t oled_partial_exec_max_ms;
    uint32_t oled_partial_over_10ms_count;
} ControlTimingStats;

void ControlTimingDiag_Reset(void);

void ControlTimingDiag_ControlBegin(uint32_t now_ms);
void ControlTimingDiag_ControlEnd(uint32_t now_ms);

void ControlTimingDiag_RecordCatchup(uint32_t extra_steps);
void ControlTimingDiag_RecordBacklogDrop(void);

void ControlTimingDiag_RecordCsvDuration(uint32_t duration_ms);
void ControlTimingDiag_RecordOledDuration(uint32_t duration_ms);
void ControlTimingDiag_RecordOledPartialDuration(uint32_t duration_ms);

const ControlTimingStats *ControlTimingDiag_GetStats(void);

void ControlTimingDiag_PrintConfig(const char *case_name,
                                   uint8_t oled_enabled,
                                   uint8_t csv_enabled);

void ControlTimingDiag_PrintUart0(const char *case_name,
                                  uint8_t oled_enabled,
                                  uint8_t csv_enabled,
                                  uint32_t requested_duration_ms,
                                  uint32_t actual_duration_ms,
                                  uint8_t task_stopped_early);

#endif

#include "Hardware/Diagnostics/ControlTimingDiag.h"

#include "Hardware/StarFlash.h"
#include "Public/Board/board.h"

static ControlTimingStats g_timing_stats;
static uint32_t g_last_control_start_ms;
static uint32_t g_current_control_start_ms;
static uint8_t g_has_last_control_start;

static void Diag_SendChar(char ch)
{
    uart0_send_char(ch);
    StarFlash_SendByte((uint8_t)ch);
}

static void Diag_SendString(const char *text)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        Diag_SendChar(*text++);
    }
}

static void Diag_SendU32(uint32_t value)
{
    char digits[10];
    uint8_t count = 0U;

    do
    {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    while (count != 0U)
    {
        Diag_SendChar(digits[--count]);
    }
}

static void Diag_SendU64(uint64_t value)
{
    char digits[20];
    uint8_t count = 0U;

    do
    {
        digits[count++] = (char)('0' + (uint8_t)(value % 10ULL));
        value /= 10ULL;
    } while (value != 0ULL);

    while (count != 0U)
    {
        Diag_SendChar(digits[--count]);
    }
}

static void Diag_SendKeyString(const char *key, const char *value)
{
    Diag_SendString(key);
    Diag_SendChar('=');
    Diag_SendString(value);
    Diag_SendString("\r\n");
}

static void Diag_SendKeyU32(const char *key, uint32_t value)
{
    Diag_SendString(key);
    Diag_SendChar('=');
    Diag_SendU32(value);
    Diag_SendString("\r\n");
}

static void Diag_SendKeyU64(const char *key, uint64_t value)
{
    Diag_SendString(key);
    Diag_SendChar('=');
    Diag_SendU64(value);
    Diag_SendString("\r\n");
}

static void Diag_RecordPeriodBucket(uint32_t period_ms)
{
    if (period_ms == 0U)
    {
        g_timing_stats.period_0ms_count++;
    }
    else if (period_ms <= 10U)
    {
        g_timing_stats.period_10ms_count++;
    }
    else if (period_ms <= 20U)
    {
        g_timing_stats.period_20ms_count++;
    }
    else if (period_ms <= 30U)
    {
        g_timing_stats.period_30ms_count++;
    }
    else
    {
        g_timing_stats.period_over_30ms_count++;
    }
}

static void Diag_RecordExecBucket(uint32_t exec_ms)
{
    if (exec_ms == 0U)
    {
        g_timing_stats.exec_0ms_count++;
    }
    else if (exec_ms <= 10U)
    {
        g_timing_stats.exec_10ms_count++;
    }
    else if (exec_ms <= 20U)
    {
        g_timing_stats.exec_20ms_count++;
    }
    else
    {
        g_timing_stats.exec_over_20ms_count++;
    }
}

void ControlTimingDiag_Reset(void)
{
    g_timing_stats.control_count = 0U;
    g_timing_stats.period_sample_count = 0U;
    g_timing_stats.period_min_ms = 0xFFFFFFFFU;
    g_timing_stats.period_max_ms = 0U;
    g_timing_stats.period_sum_ms = 0ULL;
    g_timing_stats.period_0ms_count = 0U;
    g_timing_stats.period_10ms_count = 0U;
    g_timing_stats.period_20ms_count = 0U;
    g_timing_stats.period_30ms_count = 0U;
    g_timing_stats.period_over_30ms_count = 0U;
    g_timing_stats.exec_sample_count = 0U;
    g_timing_stats.exec_max_ms = 0U;
    g_timing_stats.exec_0ms_count = 0U;
    g_timing_stats.exec_10ms_count = 0U;
    g_timing_stats.exec_20ms_count = 0U;
    g_timing_stats.exec_over_20ms_count = 0U;
    g_timing_stats.catchup_event_count = 0U;
    g_timing_stats.catchup_step_count = 0U;
    g_timing_stats.backlog_drop_event_count = 0U;
    g_timing_stats.csv_send_count = 0U;
    g_timing_stats.csv_exec_max_ms = 0U;
    g_timing_stats.csv_over_10ms_count = 0U;
    g_timing_stats.oled_refresh_count = 0U;
    g_timing_stats.oled_exec_max_ms = 0U;
    g_timing_stats.oled_over_10ms_count = 0U;
    g_timing_stats.oled_partial_write_count = 0U;
    g_timing_stats.oled_partial_exec_max_ms = 0U;
    g_timing_stats.oled_partial_over_10ms_count = 0U;
    g_last_control_start_ms = 0U;
    g_current_control_start_ms = 0U;
    g_has_last_control_start = 0U;
}

void ControlTimingDiag_ControlBegin(uint32_t now_ms)
{
    g_timing_stats.control_count++;
    g_current_control_start_ms = now_ms;

    if (g_has_last_control_start != 0U)
    {
        uint32_t period_ms = now_ms - g_last_control_start_ms;

        g_timing_stats.period_sample_count++;
        if (period_ms < g_timing_stats.period_min_ms)
        {
            g_timing_stats.period_min_ms = period_ms;
        }
        if (period_ms > g_timing_stats.period_max_ms)
        {
            g_timing_stats.period_max_ms = period_ms;
        }
        g_timing_stats.period_sum_ms += (uint64_t)period_ms;
        Diag_RecordPeriodBucket(period_ms);
    }

    g_last_control_start_ms = now_ms;
    g_has_last_control_start = 1U;
}

void ControlTimingDiag_ControlEnd(uint32_t now_ms)
{
    uint32_t exec_ms = now_ms - g_current_control_start_ms;

    g_timing_stats.exec_sample_count++;
    if (exec_ms > g_timing_stats.exec_max_ms)
    {
        g_timing_stats.exec_max_ms = exec_ms;
    }
    Diag_RecordExecBucket(exec_ms);
}

void ControlTimingDiag_RecordCatchup(uint32_t extra_steps)
{
    if (extra_steps == 0U)
    {
        return;
    }

    g_timing_stats.catchup_event_count++;
    g_timing_stats.catchup_step_count += extra_steps;
}

void ControlTimingDiag_RecordBacklogDrop(void)
{
    g_timing_stats.backlog_drop_event_count++;
}

void ControlTimingDiag_RecordCsvDuration(uint32_t duration_ms)
{
    g_timing_stats.csv_send_count++;
    if (duration_ms > g_timing_stats.csv_exec_max_ms)
    {
        g_timing_stats.csv_exec_max_ms = duration_ms;
    }
    if (duration_ms > 10U)
    {
        g_timing_stats.csv_over_10ms_count++;
    }
}

void ControlTimingDiag_RecordOledDuration(uint32_t duration_ms)
{
    g_timing_stats.oled_refresh_count++;
    if (duration_ms > g_timing_stats.oled_exec_max_ms)
    {
        g_timing_stats.oled_exec_max_ms = duration_ms;
    }
    if (duration_ms > 10U)
    {
        g_timing_stats.oled_over_10ms_count++;
    }
}

void ControlTimingDiag_RecordOledPartialDuration(uint32_t duration_ms)
{
    g_timing_stats.oled_partial_write_count++;
    if (duration_ms > g_timing_stats.oled_partial_exec_max_ms)
    {
        g_timing_stats.oled_partial_exec_max_ms = duration_ms;
    }
    if (duration_ms > 10U)
    {
        g_timing_stats.oled_partial_over_10ms_count++;
    }
}
const ControlTimingStats *ControlTimingDiag_GetStats(void)
{
    return &g_timing_stats;
}

void ControlTimingDiag_PrintConfig(const char *case_name,
                                   uint8_t oled_enabled,
                                   uint8_t csv_enabled)
{
    Diag_SendString("ARCH001_START\r\n");
    Diag_SendKeyString("case", case_name);
    Diag_SendKeyU32("oled_enabled", oled_enabled);
    Diag_SendKeyU32("csv_enabled", csv_enabled);
}

void ControlTimingDiag_PrintUart0(const char *case_name,
                                  uint8_t oled_enabled,
                                  uint8_t csv_enabled,
                                  uint32_t requested_duration_ms,
                                  uint32_t actual_duration_ms,
                                  uint8_t task_stopped_early)
{
    uint64_t period_avg_x100_ms = 0ULL;
    uint32_t period_min_ms = 0U;

    if (g_timing_stats.period_sample_count != 0U)
    {
        period_min_ms = g_timing_stats.period_min_ms;
        period_avg_x100_ms =
            (g_timing_stats.period_sum_ms * 100ULL) /
            (uint64_t)g_timing_stats.period_sample_count;
    }

    Diag_SendString("ARCH001_BEGIN\r\n");
    Diag_SendKeyString("case", case_name);
    Diag_SendKeyString("build_mode", "ARCH001_TIMING_TEST");
    Diag_SendKeyU32("oled_enabled", oled_enabled);
    Diag_SendKeyU32("csv_enabled", csv_enabled);
    Diag_SendKeyU32("requested_duration_ms", requested_duration_ms);
    Diag_SendKeyU32("actual_duration_ms", actual_duration_ms);
    Diag_SendKeyU32("task_stopped_early", task_stopped_early);
    Diag_SendKeyU32("control_count", g_timing_stats.control_count);
    Diag_SendKeyU32("period_sample_count", g_timing_stats.period_sample_count);
    Diag_SendKeyU32("period_min_ms", period_min_ms);
    Diag_SendKeyU64("period_avg_x100_ms", period_avg_x100_ms);
    Diag_SendKeyU32("period_max_ms", g_timing_stats.period_max_ms);
    Diag_SendKeyU32("period_0ms_count", g_timing_stats.period_0ms_count);
    Diag_SendKeyU32("period_10ms_count", g_timing_stats.period_10ms_count);
    Diag_SendKeyU32("period_20ms_count", g_timing_stats.period_20ms_count);
    Diag_SendKeyU32("period_30ms_count", g_timing_stats.period_30ms_count);
    Diag_SendKeyU32("period_over_30ms_count",
                    g_timing_stats.period_over_30ms_count);
    Diag_SendKeyU32("exec_sample_count", g_timing_stats.exec_sample_count);
    Diag_SendKeyU32("exec_max_ms", g_timing_stats.exec_max_ms);
    Diag_SendKeyU32("exec_0ms_count", g_timing_stats.exec_0ms_count);
    Diag_SendKeyU32("exec_10ms_count", g_timing_stats.exec_10ms_count);
    Diag_SendKeyU32("exec_20ms_count", g_timing_stats.exec_20ms_count);
    Diag_SendKeyU32("exec_over_20ms_count",
                    g_timing_stats.exec_over_20ms_count);
    Diag_SendKeyU32("catchup_event_count",
                    g_timing_stats.catchup_event_count);
    Diag_SendKeyU32("catchup_step_count", g_timing_stats.catchup_step_count);
    Diag_SendKeyU32("backlog_drop_event_count",
                    g_timing_stats.backlog_drop_event_count);
    Diag_SendKeyU32("csv_send_count", g_timing_stats.csv_send_count);
    Diag_SendKeyU32("csv_exec_max_ms", g_timing_stats.csv_exec_max_ms);
    Diag_SendKeyU32("csv_over_10ms_count", g_timing_stats.csv_over_10ms_count);
    Diag_SendKeyU32("oled_refresh_count", g_timing_stats.oled_refresh_count);
    Diag_SendKeyU32("oled_exec_max_ms", g_timing_stats.oled_exec_max_ms);
    Diag_SendKeyU32("oled_over_10ms_count",
                    g_timing_stats.oled_over_10ms_count);
    Diag_SendKeyU32("oled_partial_write_count",
                    g_timing_stats.oled_partial_write_count);
    Diag_SendKeyU32("oled_partial_exec_max_ms",
                    g_timing_stats.oled_partial_exec_max_ms);
    Diag_SendKeyU32("oled_partial_over_10ms_count",
                    g_timing_stats.oled_partial_over_10ms_count);
    Diag_SendString("ARCH001_END\r\n");
}

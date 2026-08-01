#include "Application/TaskArch001Timing.h"

#include <stdint.h>

#include "Application/OledRealtimeTime.h"
#include "Application/Task3_LinkedOperation.h"
#include "Hardware/Diagnostics/ControlTimingDiag.h"
#include "Hardware/Motor.h"
#include "Hardware/OledDisplay.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Public/Board/board.h"

#define ARCH001_TEST_DURATION_MS 10000U
#define ARCH001_OLED_REFRESH_MS   500U

typedef enum
{
    ARCH001_CASE_A_FULL = 0,
    ARCH001_CASE_B_NO_CSV,
    ARCH001_CASE_C_NO_OLED,
    ARCH001_CASE_D_MIN_LOAD,
    ARCH001_CASE_E_REALTIME_TIME
} Arch001TestCase;

#define ARCH001_SELECTED_CASE ARCH001_CASE_E_REALTIME_TIME

static void Arch001_AppendU32(char *text, uint8_t *pos, uint32_t value)
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
        text[(*pos)++] = digits[--count];
    }
    text[*pos] = '\0';
}

static void Arch001_FormatElapsedLine(char *text, uint32_t elapsed_ms)
{
    uint8_t pos = 0U;

    text[pos++] = 'T';
    text[pos++] = 'I';
    text[pos++] = 'M';
    text[pos++] = 'E';
    text[pos++] = ':';
    text[pos++] = ' ';
    Arch001_AppendU32(text, &pos, elapsed_ms / 1000U);
    text[pos++] = 'S';
    text[pos] = '\0';
}

static void Arch001_FormatElapsedTenthsLine(char *text, uint32_t elapsed_ms)
{
    uint32_t elapsed_tenths = elapsed_ms / 100U;
    uint32_t whole_seconds;
    uint32_t tenths;
    uint8_t pos = 0U;

    if (elapsed_tenths > 9999U)
    {
        elapsed_tenths = 9999U;
    }

    whole_seconds = elapsed_tenths / 10U;
    tenths = elapsed_tenths % 10U;

    text[pos++] = 'T';
    text[pos++] = 'I';
    text[pos++] = 'M';
    text[pos++] = 'E';
    text[pos++] = ':';
    text[pos++] = ' ';
    text[pos++] = (char)('0' + ((whole_seconds / 100U) % 10U));
    text[pos++] = (char)('0' + ((whole_seconds / 10U) % 10U));
    text[pos++] = (char)('0' + (whole_seconds % 10U));
    text[pos++] = '.';
    text[pos++] = (char)('0' + tenths);
    text[pos++] = 's';
    text[pos] = '\0';
}

static Arch001TestCase Arch001_GetSelectedCase(void)
{
    return (Arch001TestCase)ARCH001_SELECTED_CASE;
}

static const char *Arch001_CaseName(Arch001TestCase test_case)
{
    switch (test_case)
    {
        case ARCH001_CASE_B_NO_CSV:
            return "B_NO_CSV";

        case ARCH001_CASE_C_NO_OLED:
            return "C_NO_OLED";

        case ARCH001_CASE_D_MIN_LOAD:
            return "D_MIN_LOAD";

        case ARCH001_CASE_E_REALTIME_TIME:
            return "E_REALTIME_TIME";

        case ARCH001_CASE_A_FULL:
        default:
            return "A_FULL";
    }
}

static uint8_t Arch001_IsOledEnabled(Arch001TestCase test_case)
{
    return ((test_case == ARCH001_CASE_A_FULL) ||
            (test_case == ARCH001_CASE_B_NO_CSV) ||
            (test_case == ARCH001_CASE_E_REALTIME_TIME)) ? 1U : 0U;
}

static uint8_t Arch001_IsCsvEnabled(Arch001TestCase test_case)
{
    return ((test_case == ARCH001_CASE_A_FULL) ||
            (test_case == ARCH001_CASE_C_NO_OLED) ||
            (test_case == ARCH001_CASE_E_REALTIME_TIME)) ? 1U : 0U;
}

static uint8_t Arch001_IsRealtimeTimeEnabled(Arch001TestCase test_case)
{
    return (test_case == ARCH001_CASE_E_REALTIME_TIME) ? 1U : 0U;
}

static void Arch001_DrawTestScreen(uint32_t elapsed_ms, const char *case_name)
{
    char time_line[16];

    Arch001_FormatElapsedLine(time_line, elapsed_ms);
    OledDisplay_Clear();
    OledDisplay_WriteLine(0U, "ARCH001 TEST");
    OledDisplay_WriteLine(2U, case_name);
    OledDisplay_WriteLine(4U, time_line);
    OledDisplay_WriteLine(6U, "TIMING RUN");
    OledDisplay_WriteLine(7U, "WAIT RESULT");
    OledDisplay_Update();
}

static void Arch001_DrawRealtimeStartScreen(const char *case_name)
{
    OledDisplay_Clear();
    OledDisplay_WriteLine(0U, "ARCH001 TEST");
    OledDisplay_WriteLine(2U, case_name);
    OledDisplay_WriteLine(4U, "TIME: 000s");
    OledDisplay_WriteLine(6U, "TIMING RUN");
    OledDisplay_WriteLine(7U, "PARTIAL OLED");
    OledDisplay_Update();
}

static void Arch001_DrawRealtimeFinalScreen(uint32_t elapsed_ms,
                                            const char *case_name)
{
    char time_line[16];

    Arch001_FormatElapsedTenthsLine(time_line, elapsed_ms);
    OledDisplay_Clear();
    OledDisplay_WriteLine(0U, "ARCH001 DONE");
    OledDisplay_WriteLine(2U, case_name);
    OledDisplay_WriteLine(4U, time_line);
    OledDisplay_WriteLine(6U, "RESULT SENT");
    OledDisplay_Update();
}

void TaskArch001Timing_Run(void)
{
    Arch001TestCase test_case = Arch001_GetSelectedCase();
    const char *case_name = Arch001_CaseName(test_case);
    uint8_t oled_enabled = Arch001_IsOledEnabled(test_case);
    uint8_t csv_enabled = Arch001_IsCsvEnabled(test_case);
    uint8_t realtime_time_enabled = Arch001_IsRealtimeTimeEnabled(test_case);
    uint32_t start_ms;
    uint32_t actual_duration_ms;
    uint32_t last_oled_ms = 0U;
    uint8_t task_stopped_early = 0U;

    Motor_Coast();
    SpeedPI_Reset();

    if (oled_enabled != 0U)
    {
        OledDisplay_Init();
        if (realtime_time_enabled != 0U)
        {
            Arch001_DrawRealtimeStartScreen(case_name);
        }
    }

    ControlTimingDiag_Reset();
    ControlTimingDiag_PrintConfig(case_name, oled_enabled, csv_enabled);
    Task3_LinkedOperation_SetDebugEnabled(csv_enabled);
    start_ms = board_millis();
    Task3_LinkedOperation_StartMode(start_ms, TASK3_RUN_ONE_LAP);
    if (realtime_time_enabled != 0U)
    {
        OledRealtimeTime_Init(start_ms);
    }

    while ((board_millis() - start_ms) < ARCH001_TEST_DURATION_MS)
    {
        uint32_t now_ms = board_millis();
        uint32_t elapsed_ms;

        Task3_LinkedOperation_Update(now_ms);
        if (Task3_LinkedOperation_IsRunning() == 0U)
        {
            task_stopped_early = 1U;
            break;
        }

        if (realtime_time_enabled != 0U)
        {
            OledRealtimeTime_Request(now_ms);
            OledRealtimeTime_ProcessOneStep();
        }
        else if (oled_enabled != 0U)
        {
            elapsed_ms = now_ms - start_ms;
            if ((now_ms - last_oled_ms) >= ARCH001_OLED_REFRESH_MS)
            {
                uint32_t oled_start_ms = board_millis();

                Arch001_DrawTestScreen(elapsed_ms, case_name);
                ControlTimingDiag_RecordOledDuration(board_millis() -
                                                     oled_start_ms);
                last_oled_ms = now_ms;
            }
        }

        if (realtime_time_enabled != 0U)
        {
            delay_ms(1U);
        }
        else
        {
            delay_ms(5U);
        }
    }

    actual_duration_ms = board_millis() - start_ms;
    Task3_LinkedOperation_Stop();
    SpeedPI_Reset();
    Motor_Coast();
    if (realtime_time_enabled != 0U)
    {
        OledRealtimeTime_ForceFinal(actual_duration_ms);
        OledRealtimeTime_Reset();
    }
    delay_ms(500U);
    ControlTimingDiag_PrintUart0(case_name,
                                 oled_enabled,
                                 csv_enabled,
                                 ARCH001_TEST_DURATION_MS,
                                 actual_duration_ms,
                                 task_stopped_early);
    if ((realtime_time_enabled != 0U) && (oled_enabled != 0U))
    {
        Arch001_DrawRealtimeFinalScreen(actual_duration_ms, case_name);
    }

    while (1)
    {
        Motor_Coast();
        delay_ms(100U);
    }
}
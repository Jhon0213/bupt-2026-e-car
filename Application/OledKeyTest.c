#include "Application/OledKeyTest.h"
#include "Application/BuildConfig.h"
#include "Application/OledRealtimeTime.h"
#include "Application/Task3_LinkedOperation.h"
#include "Communication/VehicleStatePublisher.h"

#include "Hardware/Gray.h"
#include "Hardware/KeyInput.h"
#include "Hardware/Motor.h"
#include "Hardware/OledDisplay.h"
#include "Hardware/StarFlash.h"
#include "Public/Board/board.h"

#define OLED_REFRESH_MS      500U
#define GRAY_LOG_MS          100U
#define I2C_SCAN_LOG_MS     1000U

typedef enum
{
    MENU_TASK1_STANDBY = 0,
    MENU_TASK2_ONE_LAP,
    MENU_TASK3_B_PLUS_5CM,
    MENU_TASK4_ONE_LAP_ALT,
    MENU_TASK_COUNT
} MenuTask;

static MenuTask g_task;
static uint8_t g_running;
static uint32_t g_run_start_ms;
static uint32_t g_elapsed_ms;
static uint32_t g_last_screen_ms;
static uint32_t g_next_gray_log_ms;
static uint32_t g_next_scan_log_ms;

static void SendU32(uint32_t value)
{
    char digits[10];
    uint32_t count = 0U;

    do
    {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    while (count != 0U)
    {
        StarFlash_SendByte((uint8_t)digits[--count]);
    }
}

static void SendI32(int32_t value)
{
    if (value < 0)
    {
        StarFlash_SendByte((uint8_t)'-');
        SendU32((uint32_t)(-(value + 1)) + 1U);
    }
    else
    {
        SendU32((uint32_t)value);
    }
}

static char HexDigit(uint8_t value)
{
    value &= 0x0FU;
    return (value < 10U) ?
        (char)('0' + value) :
        (char)('A' + value - 10U);
}

static void SendHex8(uint8_t value)
{
    StarFlash_SendString("0x");
    StarFlash_SendByte((uint8_t)HexDigit((uint8_t)(value >> 4)));
    StarFlash_SendByte((uint8_t)HexDigit(value));
}

static void SendHex32(uint32_t value)
{
    uint8_t shift;

    StarFlash_SendString("0x");
    for (shift = 28U; shift <= 28U; shift -= 4U)
    {
        StarFlash_SendByte((uint8_t)HexDigit((uint8_t)(value >> shift)));
        if (shift == 0U)
        {
            break;
        }
    }
}

static uint8_t IsTraceTask(void)
{
    return ((g_task == MENU_TASK2_ONE_LAP) ||
            (g_task == MENU_TASK3_B_PLUS_5CM) ||
            (g_task == MENU_TASK4_ONE_LAP_ALT)) ? 1U : 0U;
}

static Task3_RunMode CurrentTraceMode(void)
{
    if (g_task == MENU_TASK3_B_PLUS_5CM)
    {
        return TASK3_RUN_B_PLUS_5CM;
    }
    if (g_task == MENU_TASK4_ONE_LAP_ALT)
    {
        return TASK3_RUN_ONE_LAP_ALT;
    }
    return TASK3_RUN_ONE_LAP;
}
static const char *TaskText(void)
{
    if (g_task == MENU_TASK2_ONE_LAP)
    {
        return "TASK2: ONE LAP";
    }
    if (g_task == MENU_TASK3_B_PLUS_5CM)
    {
        return "TASK3: B+5CM";
    }
    if (g_task == MENU_TASK4_ONE_LAP_ALT)
    {
        return "TASK4: BALL LAP";
    }
    return "TASK1: STANDBY";
}

static const char *StateText(void)
{
    if (g_running != 0U)
    {
        return "STATE: RUN";
    }
    return "STATE: WAIT";
}

static void FormatTimeLine(char *text, uint32_t elapsed_ms)
{
    uint32_t seconds = elapsed_ms / 1000U;
    uint8_t pos = 0U;

    text[pos++] = 'T';
    text[pos++] = 'I';
    text[pos++] = 'M';
    text[pos++] = 'E';
    text[pos++] = ':';
    text[pos++] = ' ';

    if (seconds >= 100U)
    {
        text[pos++] = (char)('0' + ((seconds / 100U) % 10U));
    }
    if (seconds >= 10U)
    {
        text[pos++] = (char)('0' + ((seconds / 10U) % 10U));
    }
    text[pos++] = (char)('0' + (seconds % 10U));
    text[pos++] = 'S';
    text[pos] = '\0';
}

static void FormatTask3OdoLine(char *text)
{
    int32_t count = Task3_LinkedOperation_GetOdometerCount();
    uint32_t magnitude;
    char digits[10];
    uint8_t digit_count = 0U;
    uint8_t pos = 0U;

    text[pos++] = 'O';
    text[pos++] = 'D';
    text[pos++] = 'O';
    text[pos++] = ':';
    text[pos++] = ' ';

    if (count < 0)
    {
        text[pos++] = '-';
        magnitude = (uint32_t)(-(count + 1)) + 1U;
    }
    else
    {
        magnitude = (uint32_t)count;
    }

    do
    {
        digits[digit_count++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    } while (magnitude != 0U);

    while ((digit_count != 0U) && (pos < 15U))
    {
        text[pos++] = digits[--digit_count];
    }
    text[pos] = '\0';
}
static void DrawScreen(uint32_t now_ms)
{
    char time_line[16];
    char odo_line[16];
    uint32_t elapsed_ms = g_elapsed_ms;

    FormatTimeLine(time_line, elapsed_ms);
    FormatTask3OdoLine(odo_line);

    OledDisplay_Clear();
    OledDisplay_WriteLine(0U, TaskText());
    if (IsTraceTask() != 0U)
    {
        OledDisplay_WriteLine(2U, Task3_LinkedOperation_GetSegmentText());
        OledDisplay_WriteLine(4U, odo_line);
        OledDisplay_WriteLine(6U, time_line);
    }
    else
    {
        OledDisplay_WriteLine(2U, "MOTOR: BRAKE");
        OledDisplay_WriteLine(4U, StateText());
        OledDisplay_WriteLine(6U, time_line);
    }
    OledDisplay_WriteLine(7U, (g_running != 0U) ?
                              "K1 STOP" :
                              "K1 START K2 NEXT");
    OledDisplay_Update();
}

static void DrawTraceRunScreen(void)
{
    OledDisplay_Clear();
    OledDisplay_WriteLine(0U, TaskText());
    OledDisplay_WriteLine(2U, "STATE: RUN");
    OledDisplay_WriteLine(4U, "TIME: 000s");
    OledDisplay_WriteLine(6U, "ODO: --------");
    OledDisplay_WriteLine(7U, "K1 STOP");
    OledDisplay_Update();
}
static void SendGraySample(uint32_t now_ms)
{
    Gray_Update();

    StarFlash_SendString("gray,t=");
    SendU32(now_ms);
    StarFlash_SendString(",ok=");
    SendU32(Gray_IsI2COk());
    StarFlash_SendString(",stage=");
    SendU32(Gray_GetI2CStage());
    StarFlash_SendString(",st=");
    SendHex32(Gray_GetI2CStatus());
    StarFlash_SendString(",raw=");
    SendHex8(Gray_GetRaw());
    StarFlash_SendString(",mask=");
    SendHex8(Gray_GetBlackMask());
    StarFlash_SendString(",err=");
    SendI32(Gray_GetError());
    StarFlash_SendString(",lost=");
    SendU32(Gray_IsLost());
    StarFlash_SendString("\r\n");
}

static void SendI2CScan(uint32_t now_ms)
{
    uint8_t addr;
    uint8_t found = 0U;

    StarFlash_SendString("scan,t=");
    SendU32(now_ms);
    StarFlash_SendString(",addr=");

    for (addr = 0x08U; addr <= 0x77U; addr++)
    {
        if (Gray_I2CProbeAddress(addr) != 0U)
        {
            if (found != 0U)
            {
                StarFlash_SendByte((uint8_t)' ');
            }
            SendHex8(addr);
            found = 1U;
        }
    }

    if (found == 0U)
    {
        StarFlash_SendString("none");
    }

    StarFlash_SendString(",last_stage=");
    SendU32(Gray_GetI2CStage());
    StarFlash_SendString(",last_st=");
    SendHex32(Gray_GetI2CStatus());
    StarFlash_SendString("\r\n");
}

static void StartTask2I2CDiag(uint32_t now_ms)
{
    Gray_Init();
    g_running = 1U;
    g_run_start_ms = now_ms;
    g_next_gray_log_ms = now_ms;
    g_next_scan_log_ms = now_ms;
    StarFlash_SendString("TASK2_SOFT_I2C_DIAG_START\r\n");
    StarFlash_SendString("stage:0=ok,2=scl_stuck,3=start_stuck,4=addr_w_nack,5=reg_nack,6=addr_r_nack\r\n");
    StarFlash_SendString("fmt:scan,t,addr,last_stage,last_st\r\n");
    StarFlash_SendString("fmt:gray,t,ok,stage,st,raw,mask,err,lost\r\n");
}

static void StopCurrentTask(void)
{
    uint32_t final_ms = board_millis();

    g_running = 0U;
    if (IsTraceTask() != 0U)
    {
        Task3_LinkedOperation_StopByUser();
        OledRealtimeTime_Reset();
        g_elapsed_ms = final_ms - g_run_start_ms;
    }
    Motor_Coast();
}

static void StartCurrentTask(uint32_t now_ms)
{
    Motor_Coast();
    g_elapsed_ms = 0U;

    if (IsTraceTask() != 0U)
    {
        uint32_t start_ms;

        DrawTraceRunScreen();
        start_ms = board_millis();
        g_running = 1U;
        g_run_start_ms = start_ms;
        g_last_screen_ms = start_ms;
        Task3_LinkedOperation_StartMode(start_ms, CurrentTraceMode());
        OledRealtimeTime_Init(start_ms);
        return;
    }

    (void)now_ms;
    g_running = 0U;
    Motor_Brake();
#if CONTROL_DEBUG_PRINT_ENABLE
    StarFlash_SendString("TASK1_STANDBY,MOTOR_BRAKE\r\n");
#endif
}

static void NextTask(void)
{
    if (g_task == MENU_TASK1_STANDBY)
    {
        g_task = MENU_TASK2_ONE_LAP;
    }
    else if (g_task == MENU_TASK2_ONE_LAP)
    {
        g_task = MENU_TASK3_B_PLUS_5CM;
    }
    else if (g_task == MENU_TASK3_B_PLUS_5CM)
    {
        g_task = MENU_TASK4_ONE_LAP_ALT;
    }
    else
    {
        g_task = MENU_TASK1_STANDBY;
    }
}

static void UpdateRunningTask(uint32_t now_ms)
{
    if (g_running == 0U)
    {
        Motor_Coast();
        return;
    }

    g_elapsed_ms = now_ms - g_run_start_ms;

    if (IsTraceTask() != 0U)
    {
        Task3_LinkedOperation_Update(now_ms);
        if (Task3_LinkedOperation_IsRunning() == 0U)
        {
            uint32_t final_ms = board_millis();

            g_running = 0U;
            Task3_LinkedOperation_Stop();
            Motor_Coast();
            g_elapsed_ms = final_ms - g_run_start_ms;
            OledRealtimeTime_Reset();
            DrawScreen(final_ms);
            g_last_screen_ms = final_ms;
            return;
        }
        OledRealtimeTime_Request(now_ms);
        OledRealtimeTime_ProcessOneStep();
        return;
    }

    g_running = 0U;
    Motor_Brake();
}

void OledKeyTest_Run(void)
{
    uint32_t now_ms;

    Motor_Coast();
    KeyInput_Init();
    OledDisplay_Init();

    g_task = MENU_TASK1_STANDBY;
    g_running = 0U;
    g_run_start_ms = 0U;
    g_last_screen_ms = 0U;
    g_next_gray_log_ms = 0U;
    g_next_scan_log_ms = 0U;

#if CONTROL_DEBUG_PRINT_ENABLE
    StarFlash_SendString("OLED_MENU_READY,T1_STANDBY,T2_ONE_LAP,T3_B_PLUS_5CM,T4_BALL_LAP\r\n");
#endif
    DrawScreen(board_millis());

    while (1)
    {
        uint8_t events;

        now_ms = board_millis();
        events = KeyInput_Update(now_ms);

        if ((events & KEY_INPUT_K1_PRESSED) != 0U)
        {
            if (g_running != 0U)
            {
                StopCurrentTask();
                DrawScreen(board_millis());
                g_last_screen_ms = board_millis();
            }
            else
            {
                StartCurrentTask(now_ms);
                if (g_running == 0U)
                {
                    DrawScreen(board_millis());
                    g_last_screen_ms = board_millis();
                }
            }
        }

        if (((events & KEY_INPUT_K2_PRESSED) != 0U) && (g_running == 0U))
        {
            NextTask();
            DrawScreen(now_ms);
            g_last_screen_ms = now_ms;
        }

        now_ms = board_millis();
        UpdateRunningTask(now_ms);
        VehicleStatePublisher_Process(board_millis());

        now_ms = board_millis();
        if ((g_running == 0U) &&
            ((now_ms - g_last_screen_ms) >= OLED_REFRESH_MS))
        {
            DrawScreen(now_ms);
            g_last_screen_ms = now_ms;
        }

        if (g_running != 0U)
        {
            delay_ms(1U);
        }
        else
        {
            delay_ms(5U);
        }
    }
}




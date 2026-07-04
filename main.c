#include "ti_msp_dl_config.h"

#include "Hardware/Bluetooth.h"
#include "Hardware/Encoder.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#include <stdint.h>
#include <stdio.h>

#define APP_MODE_OPEN_LOOP_LOG (1U)
#define APP_MODE APP_MODE_OPEN_LOOP_LOG

static void Log_OpenLoopData(uint32_t t_ms, uint8_t test_id, int pwm_cmd)
{
    Bluetooth_Printf("%lu,OL,%u,%d,%d,%d,%ld,%ld\r\n",
                     (unsigned long)t_ms,
                     (unsigned int)test_id,
                     pwm_cmd,
                     (int)Encoder_GetLeftSpeed(),
                     (int)Encoder_GetRightSpeed(),
                     (long)Encoder_GetLeftCount(),
                     (long)Encoder_GetRightCount());
}

static void OpenLoop_LogTest_OnePWM(void)
{
    const int pwm = 300;
    const uint8_t test_id = 1U;
    uint32_t t_ms = 0U;
    uint32_t print_ms = 0U;

    Bluetooth_SendString("t_ms,mode,test_id,pwm_cmd,left_speed,right_speed,left_count,right_count\r\n");

    Encoder_ClearCount();
    move(pwm, pwm);

    while (t_ms <= 2000U)
    {
        delay_ms(20);
        t_ms += 20U;
        print_ms += 20U;

        if (print_ms >= 100U)
        {
            print_ms = 0U;
            Log_OpenLoopData(t_ms, test_id, pwm);
        }
    }

    move(0, 0);

    while (1)
    {
        Bluetooth_SendString("MARK,OL_DONE,1,300\r\n");
        delay_ms(1000);
    }
}

int main(void)
{
    board_init();
    Motor_Init();
    Encoder_Init();
    Bluetooth_Init();

#if APP_MODE == APP_MODE_OPEN_LOOP_LOG
    OpenLoop_LogTest_OnePWM();
#else
    while (1)
    {
        delay_ms(1000);
    }
#endif
}

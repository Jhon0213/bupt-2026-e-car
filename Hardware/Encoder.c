#include "Encoder.h"

#include "Public/Board/board.h"

volatile encoder_t motor_1;
volatile encoder_t motor_2;

static uint32_t EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void ExitCritical(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

void Encoder_Init(void)
{
    NVIC_EnableIRQ(ENCAM1_INST_INT_IRQN);
    DL_TimerA_startCounter(ENCAM1_INST);
    NVIC_EnableIRQ(ENCAM2_INST_INT_IRQN);
    DL_TimerG_startCounter(ENCAM2_INST);
    Encoder_ClearCount();
}

/* motor_1 = right encoder, motor_2 = left encoder. */
int32_t Encoder_GetLeftCount(void)
{
    uint32_t primask;
    int32_t count;

    primask = EnterCritical();
    count = motor_2.countnum;
    ExitCritical(primask);

    return count;
}

int32_t Encoder_GetRightCount(void)
{
    uint32_t primask;
    int32_t count;

    primask = EnterCritical();
    count = motor_1.countnum;
    ExitCritical(primask);

    return count;
}

void Encoder_ClearCount(void)
{
    uint32_t primask;

    primask = EnterCritical();
    motor_1.countnum = 0;
    motor_2.countnum = 0;
    motor_1.lastcount = 0;
    motor_2.lastcount = 0;
    motor_1.speed_raw = 0.0f;
    motor_2.speed_raw = 0.0f;
    motor_1.speed = 0.0f;
    motor_2.speed = 0.0f;
    ExitCritical(primask);
}

float Encoder_GetLeftSpeed(void)
{
    uint32_t primask;
    float speed;

    primask = EnterCritical();
    speed = motor_2.speed;
    ExitCritical(primask);

    return speed;
}

float Encoder_GetRightSpeed(void)
{
    uint32_t primask;
    float speed;

    primask = EnterCritical();
    speed = motor_1.speed;
    ExitCritical(primask);

    return speed;
}

void TIMA1_IRQHandler(void)
{
    switch (DL_TimerA_getPendingInterrupt(ENCAM1_INST))
    {
        case DL_TIMERA_IIDX_CC0_DN:
            if (DL_GPIO_readPins(ENC_B_PORT, ENC_B_M1_PIN) != 0U)
            {
                motor_2.countnum--;
            }
            else
            {
                motor_2.countnum++;
            }
            break;

        default:
            break;
    }
}

void TIMG8_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(ENCAM2_INST))
    {
        case DL_TIMERG_IIDX_CC0_DN:
            if (DL_GPIO_readPins(ENC_B_PORT, ENC_B_M2_PIN) != 0U)
            {
                motor_1.countnum++;
            }
            else
            {
                motor_1.countnum--;
            }
            break;

        default:
            break;
    }
}

void Encoder_CalcSpeed_M1(void)
{
    int32_t delta = motor_1.countnum - motor_1.lastcount;

    motor_1.lastcount = motor_1.countnum;
    motor_1.speed_raw = (float)delta;
    motor_1.speed = motor_1.speed_raw * 6000.0f / PULSE_PER_CYCLE;
}

void Encoder_CalcSpeed_M2(void)
{
    int32_t delta = motor_2.countnum - motor_2.lastcount;

    motor_2.lastcount = motor_2.countnum;
    motor_2.speed_raw = (float)delta;
    motor_2.speed = motor_2.speed_raw * 6000.0f / PULSE_PER_CYCLE;
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            board_time_advance_ms(10U);
            Encoder_CalcSpeed_M1();
            Encoder_CalcSpeed_M2();
            board_control_tick_notify();
            break;

        default:
            break;
    }
}

void TIMER_1_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_1_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            break;

        default:
            break;
    }
}

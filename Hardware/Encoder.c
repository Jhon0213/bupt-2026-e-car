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
    uint8_t i;

    primask = EnterCritical();
    motor_1.countnum = 0;
    motor_2.countnum = 0;
    motor_1.lastcount = 0;
    motor_2.lastcount = 0;
    motor_1.delta_count = 0;
    motor_2.delta_count = 0;
    motor_1.speed_raw = 0.0f;
    motor_2.speed_raw = 0.0f;
    motor_1.speed = 0.0f;
    motor_2.speed = 0.0f;
    motor_1.speed_record_sum = 0.0f;
    motor_2.speed_record_sum = 0.0f;
    motor_1.record_index = 0U;
    motor_2.record_index = 0U;
    motor_1.record_count = 0U;
    motor_2.record_count = 0U;
    for (i = 0U; i < SPEED_RECORD_NUM; i++)
    {
        motor_1.speed_Record[i] = 0.0f;
        motor_2.speed_Record[i] = 0.0f;
    }
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

float Encoder_GetLeftRawSpeed(void)
{
    uint32_t primask;
    float speed;

    primask = EnterCritical();
    speed = motor_2.speed_raw;
    ExitCritical(primask);

    return speed;
}

float Encoder_GetRightRawSpeed(void)
{
    uint32_t primask;
    float speed;

    primask = EnterCritical();
    speed = motor_1.speed_raw;
    ExitCritical(primask);

    return speed;
}

int32_t Encoder_GetLeftDeltaCount(void)
{
    uint32_t primask;
    int32_t delta_count;

    primask = EnterCritical();
    delta_count = motor_2.delta_count;
    ExitCritical(primask);

    return delta_count;
}

int32_t Encoder_GetRightDeltaCount(void)
{
    uint32_t primask;
    int32_t delta_count;

    primask = EnterCritical();
    delta_count = motor_1.delta_count;
    ExitCritical(primask);

    return delta_count;
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

static void Encoder_UpdateSpeed(volatile encoder_t *encoder)
{
    int32_t delta = encoder->countnum - encoder->lastcount;
    float raw_rpm;

    encoder->lastcount = encoder->countnum;
    encoder->delta_count = delta;
    raw_rpm = (float)delta * 6000.0f / PULSE_PER_CYCLE;
    encoder->speed_raw = raw_rpm;

    encoder->speed_record_sum -= encoder->speed_Record[encoder->record_index];
    encoder->speed_Record[encoder->record_index] = raw_rpm;
    encoder->speed_record_sum += raw_rpm;
    encoder->record_index++;
    if (encoder->record_index >= SPEED_RECORD_NUM)
    {
        encoder->record_index = 0U;
    }
    if (encoder->record_count < SPEED_RECORD_NUM)
    {
        encoder->record_count++;
    }

    encoder->speed = encoder->speed_record_sum / (float)encoder->record_count;
}

void Encoder_CalcSpeed_M1(void)
{
    Encoder_UpdateSpeed(&motor_1);
}

void Encoder_CalcSpeed_M2(void)
{
    Encoder_UpdateSpeed(&motor_2);
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

#include "Encoder.h"

encoder_t motor_1;
encoder_t motor_2;

void Encoder_Init(void)
{
    NVIC_EnableIRQ(ENCAM1_INST_INT_IRQN);
    DL_TimerA_startCounter(ENCAM1_INST);
    NVIC_EnableIRQ(ENCAM2_INST_INT_IRQN);
    DL_TimerG_startCounter(ENCAM2_INST);
    motor_1.countnum = 0;
    motor_2.countnum = 0;
}

/* motor_1 = right encoder, motor_2 = left encoder.
 * Encoder_GetLeftCount() returns motor_2.countnum.
 * Encoder_GetRightCount() returns motor_1.countnum.
 */
int32_t Encoder_GetLeftCount(void)
{
    return motor_2.countnum;
}

int32_t Encoder_GetRightCount(void)
{
    return motor_1.countnum;
}

void Encoder_ClearCount(void)
{
    motor_1.countnum = 0;
    motor_2.countnum = 0;
    motor_1.lastcount = 0;
    motor_2.lastcount = 0;
}

float Encoder_GetLeftSpeed(void)
{
    return motor_2.speed;
}

float Encoder_GetRightSpeed(void)
{
    return motor_1.speed;
}

/* ???????????motor_2 ????????
 * ?????????妊?????? count ?????????? count ?????
 */
void TIMA1_IRQHandler(void) {
  switch (DL_TimerA_getPendingInterrupt(ENCAM1_INST)) 
  {
    case DL_TIMERA_IIDX_CC0_DN:
        if (DL_GPIO_readPins(ENC_B_PORT, ENC_B_M1_PIN) != 0) {
            motor_2.countnum--;
        } else {
            motor_2.countnum++;
        }
        break;
    default:
        break;
  }
}
 
/* ???????????motor_1 ????????
 * ?????????妊?????? count ?????????? count ?????
 */
void TIMG8_IRQHandler(void) {
  switch (DL_TimerG_getPendingInterrupt(ENCAM2_INST)) 
  {
    case DL_TIMERG_IIDX_CC0_DN:
        if (DL_GPIO_readPins(ENC_B_PORT, ENC_B_M2_PIN) != 0) {
            motor_1.countnum++;
        } else {
            motor_1.countnum--;
        }
        break;
    default:
        break;
  }
}


// ?????????? (?? TIMER_0 ?忪??快????????10ms)
void Encoder_CalcSpeed_M1(void)
{
    const float dt = 0.01f;  // 10ms
    
    int32_t delta = motor_1.countnum - motor_1.lastcount;
    motor_1.lastcount = motor_1.countnum;
    motor_1.speed_raw = (float)delta;  // ????/10ms
    
    // ???? RPM: ????/10ms ?? 100 ?? 60 ?? ????????
    motor_1.speed = motor_1.speed_raw * 6000.0f / PULSE_PER_CYCLE;
}

// ?????????? (?? TIMER_1 ?忪??快????????10ms)
void Encoder_CalcSpeed_M2(void)
{
    const float dt = 0.01f;  // 10ms
    
    int32_t delta = motor_2.countnum - motor_2.lastcount;
    motor_2.lastcount = motor_2.countnum;
    motor_2.speed_raw = (float)delta;  // ????/10ms
    
    // ???? RPM: ????/10ms ?? 100 ?? 60 ?? ????????
    motor_2.speed = motor_2.speed_raw * 6000.0f / PULSE_PER_CYCLE;
}

// ?? TIMER_0 ?忪??快?????????? (10ms)
void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) 
    {
        case DL_TIMER_IIDX_ZERO:
            Encoder_CalcSpeed_M1();  // ???????
            break;
        default:
            break;
    }
}

// ?? TIMER_1 ?忪??快?????????? (10ms)
void TIMER_1_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_1_INST)) 
    {
        case DL_TIMER_IIDX_ZERO:
            Encoder_CalcSpeed_M2();  // ???????
            break;
        default:
            break;
    }
}
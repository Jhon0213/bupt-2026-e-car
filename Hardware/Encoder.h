#ifndef ENCODER_H
#define ENCODER_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

#define MOTOR_SPEED_RERATIO 28.0f
#define PULSE_PRE_ROUND 500.0f
#define ENCODER_MULTIPLE 1.0f
#define PULSE_PER_CYCLE  (MOTOR_SPEED_RERATIO * PULSE_PRE_ROUND * ENCODER_MULTIPLE)
#define RADIUS_OF_TYRE 3.3f
#define LINE_SPEED_C  (RADIUS_OF_TYRE * 2.0f * 3.14f)
#define SPEED_RECORD_NUM 5U

typedef struct
{
    int32_t countnum;
    int32_t lastcount;
    int32_t delta_count;
    float speed;
    float speed_raw;
    float speed_Record[SPEED_RECORD_NUM];
    float speed_record_sum;
    uint8_t record_index;
    uint8_t record_count;
} encoder_t;

extern volatile encoder_t motor_1;
extern volatile encoder_t motor_2;

void Encoder_Init(void);
void Encoder_CalcSpeed_M1(void);
void Encoder_CalcSpeed_M2(void);

/* motor_1 = right encoder, motor_2 = left encoder */
int32_t Encoder_GetLeftCount(void);
int32_t Encoder_GetRightCount(void);
void Encoder_ClearCount(void);
float Encoder_GetLeftSpeed(void);
float Encoder_GetRightSpeed(void);
float Encoder_GetLeftRawSpeed(void);
float Encoder_GetRightRawSpeed(void);
int32_t Encoder_GetLeftDeltaCount(void);
int32_t Encoder_GetRightDeltaCount(void);

#endif

#ifndef ENCODER_H
#define ENCODER_H

#include "ti_msp_dl_config.h"
#include "stdlib.h"
#include <stdint.h>
 
#define MOTOR_SPEED_RERATIO 28    //????????
#define PULSE_PRE_ROUND 13       //????????
#define ENCODER_MULTIPLE 1.0     //?????????
#define PULSE_PER_CYCLE  (MOTOR_SPEED_RERATIO*PULSE_PRE_ROUND*ENCODER_MULTIPLE)//????????
#define RADIUS_OF_TYRE 3.3  //??????? cm
#define LINE_SPEED_C  RADIUS_OF_TYRE * 2 * 3.14  //??????? cm
#define SPEED_RECORD_NUM 20  //?????????????????

typedef struct {
    uint8_t dierct;     //????
    int32_t countnum;   //??????
    int32_t lastcount;  //????¦Ì????
    float speed;        //?????? (RPM)
    float speed_raw;    //????? (????/????)
    float speed_Record[SPEED_RECORD_NUM];
    uint8_t record_index;
} encoder_t;

extern encoder_t motor_1;
extern encoder_t motor_2;

void Encoder_Init(void);
void Encoder_CalcSpeed_M1(void);  // ???????
void Encoder_CalcSpeed_M2(void);  // ???????

/* motor_1 = right encoder, motor_2 = left encoder */
int32_t Encoder_GetLeftCount(void);
int32_t Encoder_GetRightCount(void);
void Encoder_ClearCount(void);
float Encoder_GetLeftSpeed(void);
float Encoder_GetRightSpeed(void);

#endif
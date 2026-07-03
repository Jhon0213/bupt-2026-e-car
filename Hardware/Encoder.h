#ifndef ENCODER_H
#define ENCODER_H

#include "ti_msp_dl_config.h"
#include "stdlib.h"
 
#define MOTOR_SPEED_RERATIO 28    //电机减速比
#define PULSE_PRE_ROUND 13       //每圈脉冲数
#define ENCODER_MULTIPLE 1.0     //编码器倍频
#define PULSE_PER_CYCLE  (MOTOR_SPEED_RERATIO*PULSE_PRE_ROUND*ENCODER_MULTIPLE)//每圈脉冲数
#define RADIUS_OF_TYRE 3.3  //轮子直径 cm
#define LINE_SPEED_C  RADIUS_OF_TYRE * 2 * 3.14  //轮子周长 cm
#define SPEED_RECORD_NUM 20  //速度记录值，用于均值滤波

typedef struct {
    uint8_t dierct;     //方向
    int32_t countnum;   //总计数值
    int32_t lastcount;  //上一次计数值
    float speed;        //电机速度 (RPM)
    float speed_raw;    //原始速度 (脉冲/周期)
    float speed_Record[SPEED_RECORD_NUM];
    uint8_t record_index;
} encoder_t;

extern encoder_t motor_1;
extern encoder_t motor_2;

void Encoder_Init(void);
void Encoder_CalcSpeed_M1(void);  // 左轮测速
void Encoder_CalcSpeed_M2(void);  // 右轮测速

#endif
#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include "board.h"
#include "Motor.h" 
#include "Encoder.h"

// === 全局变量声明（注意是 extern） ===
extern float bias, bias_last;
extern float Initial_yaw;
extern float Kp_angle, Ki_angle;
extern float Kp_gyro, Ki_gyro, Kd_gyro;
extern float angle_error, angle_sum,last_angle_error;
extern float gyro_error, gyro_sum,last_gyro_error;

// === 函数声明 ===
void yaw_control_with_gyro(float base_speed, float target_yaw);
float limit(float val, float min_val, float max_val);

float Speed_Loop_Left(float target_speed);
float Speed_Loop_Right(float target_speed);
void Speed_Control(float target_left, float target_right);
#endif
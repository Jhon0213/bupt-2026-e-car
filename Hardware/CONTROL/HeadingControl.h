#ifndef HARDWARE_CONTROL_HEADING_CONTROL_H_
#define HARDWARE_CONTROL_HEADING_CONTROL_H_

typedef struct
{
    float target_yaw_deg;
    float actual_yaw_deg;
    float error_deg;
    float error_integral;
    float error_derivative;
    float correction_rpm;
    float base_rpm;
    float left_target_rpm;
    float right_target_rpm;
} HeadingControl_Output;

/* Position-form PID outer loop based on the referenced PID tutorial. */
void HeadingControl_Init(void);
void HeadingControl_Reset(float actual_yaw_deg);
void HeadingControl_SetGains(float kp, float ki, float kd);
void HeadingControl_SetOutputLimits(float correction_limit_rpm,
                                    float target_min_rpm,
                                    float target_max_rpm);
void HeadingControl_SetTarget(float target_yaw_deg, float base_rpm);
void HeadingControl_SetBaseRPM(float base_rpm);
void HeadingControl_Update(float actual_yaw_deg);
void HeadingControl_GetOutput(HeadingControl_Output *output);
float HeadingControl_GetCorrectionLimitRPM(void);
float HeadingControl_GetTargetMinRPM(void);
float HeadingControl_GetTargetMaxRPM(void);

#endif
#ifndef HARDWARE_CONTROL_SPEED_PI_H_
#define HARDWARE_CONTROL_SPEED_PI_H_

typedef struct
{
    float command_target_rpm;
    float control_target_rpm;
    float actual_rpm;
    float error_rpm;
    float output_pwm;
    float raw_pwm;
    float feedforward_pwm;
    float p_term;
    float integral_term;
    int pwm;
} SpeedPI_CalibrationSample;

void SpeedPI_Init(void);
void SpeedPI_Update(float left_target_rpm, float right_target_rpm);
void SpeedPI_UpdateLeftOnly(float target_rpm);
void SpeedPI_UpdateRightOnly(float target_rpm);
void SpeedPI_UpdateRightCalibrationDirect(float target_rpm,
                                          SpeedPI_CalibrationSample *sample);
void SpeedPI_UpdateLeftCalibrationDirect(float target_rpm,
                                         SpeedPI_CalibrationSample *sample);
void SpeedPI_UpdateBothCalibrationDirect(float left_target_rpm,
                                         float right_target_rpm,
                                         SpeedPI_CalibrationSample *left_sample,
                                         SpeedPI_CalibrationSample *right_sample);
void SpeedPI_Reset(void);
void SpeedPI_ResetLeftOnly(void);
void SpeedPI_ResetRightOnly(void);

int SpeedPI_GetLeftPWM(void);
int SpeedPI_GetRightPWM(void);
float SpeedPI_GetLeftRawPWM(void);
float SpeedPI_GetRightRawPWM(void);
float SpeedPI_GetLeftTarget(void);
float SpeedPI_GetRightTarget(void);
float SpeedPI_GetLeftRPM(void);
float SpeedPI_GetRightRPM(void);
float SpeedPI_GetLeftFeedforwardPWM(void);
float SpeedPI_GetLeftPTerm(void);
float SpeedPI_GetLeftIntegralTerm(void);
float SpeedPI_GetRightFeedforwardPWM(void);
float SpeedPI_GetRightPTerm(void);
float SpeedPI_GetRightIntegralTerm(void);

#endif
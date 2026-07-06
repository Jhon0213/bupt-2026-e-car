#ifndef HARDWARE_CONTROL_SPEED_PI_H_
#define HARDWARE_CONTROL_SPEED_PI_H_

void SpeedPI_Init(void);
void SpeedPI_Update(float left_target_rpm, float right_target_rpm);
void SpeedPI_BalanceForStraight(float target_rpm);
void SpeedPI_Reset(void);

int SpeedPI_GetLeftPWM(void);
int SpeedPI_GetRightPWM(void);
float SpeedPI_GetLeftTarget(void);
float SpeedPI_GetRightTarget(void);
float SpeedPI_GetLeftRPM(void);
float SpeedPI_GetRightRPM(void);

#endif

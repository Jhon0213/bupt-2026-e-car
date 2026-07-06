#ifndef HARDWARE_CONTROL_GRAY_TRACK_H_
#define HARDWARE_CONTROL_GRAY_TRACK_H_

#include <stdint.h>

typedef struct
{
    uint8_t raw;
    uint8_t black_mask;
    int16_t error;
    uint8_t line_detected;
    uint8_t lost_confirmed;
    float correction_rpm;
    float left_target_rpm;
    float right_target_rpm;
} GrayTrack_Output;
void GrayTrack_Init(void);
void GrayTrack_Reset(void);
void GrayTrack_Update(void);
void GrayTrack_GetOutput(GrayTrack_Output *output);

uint8_t GrayTrack_GetRaw(void);
uint8_t GrayTrack_GetBlackMask(void);
int16_t GrayTrack_GetError(void);
uint8_t GrayTrack_IsValid(void);
uint8_t GrayTrack_IsLost(void);
uint8_t GrayTrack_IsLostConfirmed(void);
float GrayTrack_GetCorrectionRPM(void);
float GrayTrack_GetLeftTargetRPM(void);
float GrayTrack_GetRightTargetRPM(void);

#endif

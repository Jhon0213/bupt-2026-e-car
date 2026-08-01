#ifndef COMMUNICATION_INTERBOARD_PROTOCOL_H_
#define COMMUNICATION_INTERBOARD_PROTOCOL_H_

#include <stdint.h>

#define INTERBOARD_FRAME_SIZE 18U

#define INTERBOARD_FLAG_STATE_VALID     (1U << 0)
#define INTERBOARD_FLAG_RUNNING         (1U << 1)
#define INTERBOARD_FLAG_LINE_VALID      (1U << 2)
#define INTERBOARD_FLAG_FINISHED        (1U << 3)
#define INTERBOARD_FLAG_EMERGENCY_STOP  (1U << 4)
#define INTERBOARD_FLAG_ACCEL_VALID     (1U << 5)
#define INTERBOARD_FLAG_RESERVED_6      (1U << 6)
#define INTERBOARD_FLAG_RESERVED_7      (1U << 7)

typedef enum
{
    INTERBOARD_MOTION_STOPPED  = 0U,
    INTERBOARD_MOTION_STRAIGHT = 1U,
    INTERBOARD_MOTION_TURNING  = 2U
} InterboardMotionPhase;

typedef struct
{
    uint8_t sequence;
    uint32_t timestamp_ms;
    uint8_t motion_phase;
    int16_t vehicle_speed_target_mm_s;
    int16_t vehicle_speed_measured_mm_s;
    int16_t vehicle_accel_target_mm_s2;
    int16_t vehicle_accel_measured_mm_s2;
    uint8_t flags;
} InterboardVehicleState;

void InterboardProtocol_Encode(const InterboardVehicleState *state,
                               uint8_t frame[INTERBOARD_FRAME_SIZE]);

#endif
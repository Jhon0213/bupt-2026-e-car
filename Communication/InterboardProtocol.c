#include "Communication/InterboardProtocol.h"

typedef char InterboardFrameSizeMustBe18[(INTERBOARD_FRAME_SIZE == 18U) ? 1 : -1];

static void Protocol_WriteU16LE(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void Protocol_WriteI16LE(uint8_t *buffer, int16_t value)
{
    Protocol_WriteU16LE(buffer, (uint16_t)value);
}

static void Protocol_WriteU32LE(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFUL);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFUL);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFUL);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFUL);
}

void InterboardProtocol_Encode(const InterboardVehicleState *state,
                               uint8_t frame[INTERBOARD_FRAME_SIZE])
{
    uint8_t i;
    uint8_t checksum = 0U;

    if ((state == 0) || (frame == 0))
    {
        return;
    }

    frame[0] = 0xA5U;
    frame[1] = 0x5AU;
    frame[2] = state->sequence;
    Protocol_WriteU32LE(&frame[3], state->timestamp_ms);
    frame[7] = state->motion_phase;
    Protocol_WriteI16LE(&frame[8], state->vehicle_speed_target_mm_s);
    Protocol_WriteI16LE(&frame[10], state->vehicle_speed_measured_mm_s);
    Protocol_WriteI16LE(&frame[12], state->vehicle_accel_target_mm_s2);
    Protocol_WriteI16LE(&frame[14], state->vehicle_accel_measured_mm_s2);
    frame[16] = (uint8_t)(state->flags & 0x3FU);

    for (i = 2U; i <= 16U; i++)
    {
        checksum ^= frame[i];
    }
    frame[17] = checksum;
}
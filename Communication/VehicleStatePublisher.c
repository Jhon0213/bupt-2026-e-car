#include "Communication/VehicleStatePublisher.h"

#include "Application/BuildConfig.h"

#if INTERBOARD_LINK_ENABLE

#include "Application/Task3_LinkedOperation.h"
#include "Communication/InterboardProtocol.h"
#include "Communication/InterboardUart.h"
#include "Hardware/Encoder.h"
#include "Hardware/CONTROL/SpeedPI.h"

#define INTERBOARD_WHEEL_DIAMETER_MM 65.0f
#define INTERBOARD_PI 3.1415926f
#define INTERBOARD_ACCEL_FILTER_ALPHA 0.2f
#define INTERBOARD_PUBLISH_PERIOD_S 0.020f

typedef char InterboardPublishPeriodMustBe20[(INTERBOARD_PUBLISH_PERIOD_MS == 20U) ? 1 : -1];

static uint32_t g_next_publish_ms;
static uint8_t g_sequence;
static uint8_t g_initialized;
static uint8_t g_has_previous_sample;
static uint8_t g_last_running;
static float g_previous_target_speed_mm_s;
static float g_previous_measured_speed_mm_s;
static float g_filtered_measured_accel_mm_s2;

static uint8_t TimeReached(uint32_t now_ms, uint32_t target_ms)
{
    return ((now_ms - target_ms) < 0x80000000UL) ? 1U : 0U;
}

static float RpmToMmS(float rpm)
{
    return rpm * (INTERBOARD_WHEEL_DIAMETER_MM * INTERBOARD_PI) / 60.0f;
}

static int16_t SaturateRoundI16(float value)
{
    if (value > 32767.0f)
    {
        return 32767;
    }
    if (value < -32768.0f)
    {
        return (int16_t)-32768;
    }
    if (value >= 0.0f)
    {
        return (int16_t)(value + 0.5f);
    }
    return (int16_t)(value - 0.5f);
}

static uint8_t BuildFlags(const Task3_ApplicationState *app,
                          uint8_t accel_valid)
{
    uint8_t flags = INTERBOARD_FLAG_STATE_VALID;

    if (app->running != 0U)
    {
        flags |= INTERBOARD_FLAG_RUNNING;
        if (app->line_valid != 0U)
        {
            flags |= INTERBOARD_FLAG_LINE_VALID;
        }
    }
    if (app->finished != 0U)
    {
        flags |= INTERBOARD_FLAG_FINISHED;
    }
    if ((app->stop_reason != TASK3_STOP_REASON_NONE) &&
        (app->stop_reason != TASK3_STOP_REASON_FINISHED))
    {
        flags |= INTERBOARD_FLAG_EMERGENCY_STOP;
    }
    if (accel_valid != 0U)
    {
        flags |= INTERBOARD_FLAG_ACCEL_VALID;
    }
    return flags;
}

static uint8_t GetMotionPhase(const Task3_ApplicationState *app)
{
    if (app->running == 0U)
    {
        return (uint8_t)INTERBOARD_MOTION_STOPPED;
    }
    if (app->turning != 0U)
    {
        return (uint8_t)INTERBOARD_MOTION_TURNING;
    }
    return (uint8_t)INTERBOARD_MOTION_STRAIGHT;
}

static void PublishLatest(uint32_t now_ms)
{
    Task3_ApplicationState app;
    InterboardVehicleState state;
    uint8_t frame[INTERBOARD_FRAME_SIZE];
    float target_rpm;
    float measured_rpm;
    float target_speed_mm_s;
    float measured_speed_mm_s;
    float target_accel_mm_s2 = 0.0f;
    float measured_accel_mm_s2 = 0.0f;
    uint8_t accel_valid = 0U;

    Task3_LinkedOperation_GetApplicationState(&app);
    if ((app.running != 0U) && (g_last_running == 0U))
    {
        g_has_previous_sample = 0U;
        g_filtered_measured_accel_mm_s2 = 0.0f;
    }

    if (app.running != 0U)
    {
        target_rpm = 0.5f * (SpeedPI_GetLeftTarget() +
                             SpeedPI_GetRightTarget());
    }
    else
    {
        target_rpm = 0.0f;
    }
    measured_rpm = 0.5f * (Encoder_GetLeftSpeed() +
                           Encoder_GetRightSpeed());

    target_speed_mm_s = RpmToMmS(target_rpm);
    measured_speed_mm_s = RpmToMmS(measured_rpm);

    if (g_has_previous_sample != 0U)
    {
        float raw_measured_accel_mm_s2;

        target_accel_mm_s2 = (target_speed_mm_s -
                              g_previous_target_speed_mm_s) /
                             INTERBOARD_PUBLISH_PERIOD_S;
        raw_measured_accel_mm_s2 = (measured_speed_mm_s -
                                    g_previous_measured_speed_mm_s) /
                                   INTERBOARD_PUBLISH_PERIOD_S;
        g_filtered_measured_accel_mm_s2 +=
            INTERBOARD_ACCEL_FILTER_ALPHA *
            (raw_measured_accel_mm_s2 - g_filtered_measured_accel_mm_s2);
        measured_accel_mm_s2 = g_filtered_measured_accel_mm_s2;
        accel_valid = 1U;
    }
    else
    {
        g_has_previous_sample = 1U;
        g_filtered_measured_accel_mm_s2 = 0.0f;
    }

    g_previous_target_speed_mm_s = target_speed_mm_s;
    g_previous_measured_speed_mm_s = measured_speed_mm_s;
    g_last_running = app.running;

    state.sequence = g_sequence++;
    state.timestamp_ms = now_ms;
    state.motion_phase = GetMotionPhase(&app);
    state.vehicle_speed_target_mm_s = SaturateRoundI16(target_speed_mm_s);
    state.vehicle_speed_measured_mm_s = SaturateRoundI16(measured_speed_mm_s);
    state.vehicle_accel_target_mm_s2 = SaturateRoundI16(target_accel_mm_s2);
    state.vehicle_accel_measured_mm_s2 = SaturateRoundI16(measured_accel_mm_s2);
    state.flags = BuildFlags(&app, accel_valid);

    InterboardProtocol_Encode(&state, frame);
    InterboardUart_SubmitLatest(frame);
}

void VehicleStatePublisher_Init(uint32_t now_ms)
{
    g_next_publish_ms = now_ms;
    g_sequence = 0U;
    g_initialized = 1U;
    g_has_previous_sample = 0U;
    g_last_running = 0U;
    g_previous_target_speed_mm_s = 0.0f;
    g_previous_measured_speed_mm_s = 0.0f;
    g_filtered_measured_accel_mm_s2 = 0.0f;
}

void VehicleStatePublisher_Process(uint32_t now_ms)
{
    if (g_initialized == 0U)
    {
        VehicleStatePublisher_Init(now_ms);
    }

    if (TimeReached(now_ms, g_next_publish_ms) == 0U)
    {
        return;
    }

    PublishLatest(now_ms);

    do
    {
        g_next_publish_ms += INTERBOARD_PUBLISH_PERIOD_MS;
    } while (TimeReached(now_ms, g_next_publish_ms) != 0U);
}

#else

void VehicleStatePublisher_Init(uint32_t now_ms)
{
    (void)now_ms;
}

void VehicleStatePublisher_Process(uint32_t now_ms)
{
    (void)now_ms;
}

#endif
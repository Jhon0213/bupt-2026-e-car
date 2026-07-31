#include "SpeedPI.h"

#include "Hardware/Encoder.h"
#include "Hardware/Motor.h"

#define SPEED_CONTROL_DT_SEC   0.01f
#define SPEED_LEFT_KP          0.8f
#define SPEED_LEFT_KI          4.0f
#define SPEED_RIGHT_KP         0.7f
#define SPEED_RIGHT_KI         3.0f
#define SPEED_PWM_MIN          0.0f
#define SPEED_PWM_MAX        500.0f
#define SPEED_FF_OFFSET_FULL_RPM 30.0f
#define SPEED_LEFT_PWM_STEP_MAX   12.0f
#define SPEED_RIGHT_PWM_STEP_MAX  12.0f
#define SPEED_TARGET_STEP_DISABLED 1000.0f
#define SPEED_RIGHT_TEST_TARGET_STEP_MAX 0.8f
#define SPEED_LEFT_FF_OFFSET_PWM   35.0f
#define SPEED_LEFT_FF_PWM_PER_RPM   1.0f
#define SPEED_RIGHT_FF_OFFSET_PWM  38.0f
#define SPEED_RIGHT_FF_PWM_PER_RPM  1.0f

typedef struct
{
    float target_rpm;
    float actual_rpm;
    float integral;
    float output;
    float raw_output;
    int pwm;
} SpeedPIChannel;

static SpeedPIChannel g_left;
static SpeedPIChannel g_right;

static float SpeedPI_Clamp(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static void SpeedPI_ResetChannel(SpeedPIChannel *channel)
{
    channel->target_rpm = 0.0f;
    channel->actual_rpm = 0.0f;
    channel->integral = 0.0f;
    channel->output = 0.0f;
    channel->raw_output = 0.0f;
    channel->pwm = 0;
}

static void SpeedPI_UpdateChannel(SpeedPIChannel *channel,
                                  float target_rpm,
                                  float actual_rpm,
                                  float kp,
                                  float ki,
                                  float ff_offset_pwm,
                                  float ff_pwm_per_rpm,
                                  float target_step_max,
                                  float pwm_step_max)
{
    float error;
    float feedforward;
    float proportional;
    float candidate_integral;
    float candidate_output;
    float target_delta;
    float control_target_rpm;
    float ff_offset_scale;

    channel->actual_rpm = actual_rpm;

    if (target_rpm <= 0.0f)
    {
        channel->target_rpm = 0.0f;
        channel->integral = 0.0f;
        channel->output = 0.0f;
        channel->raw_output = 0.0f;
        channel->pwm = 0;
        return;
    }

    target_delta = target_rpm - channel->target_rpm;
    channel->target_rpm += SpeedPI_Clamp(target_delta,
                                         -target_step_max,
                                         target_step_max);
    control_target_rpm = channel->target_rpm;
    error = control_target_rpm - actual_rpm;
    ff_offset_scale = SpeedPI_Clamp(control_target_rpm / SPEED_FF_OFFSET_FULL_RPM, 0.0f, 1.0f);
    feedforward = (ff_offset_pwm * ff_offset_scale) +
                  (ff_pwm_per_rpm * control_target_rpm);
    proportional = kp * error;
    candidate_integral = channel->integral + (ki * error * SPEED_CONTROL_DT_SEC);
    candidate_output = feedforward + proportional + candidate_integral;

    if (((candidate_output < SPEED_PWM_MAX) &&
         (candidate_output > SPEED_PWM_MIN)) ||
        ((candidate_output >= SPEED_PWM_MAX) && (error < 0.0f)) ||
        ((candidate_output <= SPEED_PWM_MIN) && (error > 0.0f)))
    {
        channel->integral = candidate_integral;
    }

    channel->raw_output = feedforward + proportional + channel->integral;
    candidate_output = SpeedPI_Clamp(channel->raw_output,
                                     SPEED_PWM_MIN,
                                     SPEED_PWM_MAX);
    channel->output += SpeedPI_Clamp(candidate_output - channel->output,
                                     -pwm_step_max,
                                     pwm_step_max);
    channel->output = SpeedPI_Clamp(channel->output,
                                    SPEED_PWM_MIN,
                                    SPEED_PWM_MAX);
    channel->pwm = (int)(channel->output + 0.5f);
}

void SpeedPI_Init(void)
{
    SpeedPI_Reset();
}

void SpeedPI_Update(float left_target_rpm, float right_target_rpm)
{
    SpeedPI_UpdateChannel(&g_left, left_target_rpm, Encoder_GetLeftSpeed(),
                          SPEED_LEFT_KP, SPEED_LEFT_KI,
                          SPEED_LEFT_FF_OFFSET_PWM,
                          SPEED_LEFT_FF_PWM_PER_RPM,
                          SPEED_TARGET_STEP_DISABLED,
                          SPEED_LEFT_PWM_STEP_MAX);
    SpeedPI_UpdateChannel(&g_right, right_target_rpm, Encoder_GetRightSpeed(),
                          SPEED_RIGHT_KP, SPEED_RIGHT_KI,
                          SPEED_RIGHT_FF_OFFSET_PWM,
                          SPEED_RIGHT_FF_PWM_PER_RPM,
                          SPEED_TARGET_STEP_DISABLED,
                          SPEED_RIGHT_PWM_STEP_MAX);
    move(g_left.pwm, g_right.pwm);
}

void SpeedPI_UpdateLeftOnly(float target_rpm)
{
    SpeedPI_UpdateChannel(&g_left, target_rpm, Encoder_GetLeftSpeed(),
                          SPEED_LEFT_KP, SPEED_LEFT_KI,
                          SPEED_LEFT_FF_OFFSET_PWM,
                          SPEED_LEFT_FF_PWM_PER_RPM,
                          SPEED_TARGET_STEP_DISABLED,
                          SPEED_LEFT_PWM_STEP_MAX);
    move(g_left.pwm, 0);
}

void SpeedPI_UpdateRightOnly(float target_rpm)
{
    SpeedPI_UpdateChannel(&g_right, target_rpm, Encoder_GetRightSpeed(),
                          SPEED_RIGHT_KP, SPEED_RIGHT_KI,
                          SPEED_RIGHT_FF_OFFSET_PWM,
                          SPEED_RIGHT_FF_PWM_PER_RPM,
                          SPEED_RIGHT_TEST_TARGET_STEP_MAX,
                          SPEED_RIGHT_PWM_STEP_MAX);
    move(0, g_right.pwm);
}

void SpeedPI_Reset(void)
{
    SpeedPI_ResetChannel(&g_left);
    SpeedPI_ResetChannel(&g_right);
    Motor_Coast();
}

int SpeedPI_GetLeftPWM(void) { return g_left.pwm; }
int SpeedPI_GetRightPWM(void) { return g_right.pwm; }
float SpeedPI_GetLeftRawPWM(void) { return g_left.raw_output; }
float SpeedPI_GetRightRawPWM(void) { return g_right.raw_output; }
float SpeedPI_GetLeftTarget(void) { return g_left.target_rpm; }
float SpeedPI_GetRightTarget(void) { return g_right.target_rpm; }
float SpeedPI_GetLeftRPM(void) { return g_left.actual_rpm; }
float SpeedPI_GetRightRPM(void) { return g_right.actual_rpm; }

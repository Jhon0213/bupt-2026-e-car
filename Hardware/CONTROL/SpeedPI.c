#include "SpeedPI.h"

#include "Hardware/Encoder.h"
#include "Hardware/Motor.h"

#define SPEED_PI_KP       1.5f
#define SPEED_PI_KI       0.06f
#define SPEED_PWM_MIN     0.0f
#define SPEED_PWM_MAX     500.0f

typedef struct
{
    float target_rpm;
    float actual_rpm;
    float last_error;
    float output;
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
    channel->last_error = 0.0f;
    channel->output = 0.0f;
    channel->pwm = 0;
}

static void SpeedPI_UpdateChannel(SpeedPIChannel *channel,
                                  float target_rpm,
                                  float actual_rpm)
{
    float error;
    float increment;

    channel->target_rpm = target_rpm;
    channel->actual_rpm = actual_rpm;

    if (target_rpm <= 0.0f)
    {
        channel->last_error = 0.0f;
        channel->output = 0.0f;
        channel->pwm = 0;
        return;
    }

    error = target_rpm - actual_rpm;
    increment = SPEED_PI_KP * (error - channel->last_error)
              + SPEED_PI_KI * error;
    channel->output = SpeedPI_Clamp(channel->output + increment,
                                    SPEED_PWM_MIN,
                                    SPEED_PWM_MAX);
    channel->last_error = error;
    channel->pwm = (int)(channel->output + 0.5f);
}

void SpeedPI_Init(void)
{
    SpeedPI_Reset();
}

void SpeedPI_Update(float left_target_rpm, float right_target_rpm)
{
    SpeedPI_UpdateChannel(&g_left, left_target_rpm, Encoder_GetLeftSpeed());
    SpeedPI_UpdateChannel(&g_right, right_target_rpm, Encoder_GetRightSpeed());
    move(g_left.pwm, g_right.pwm);
}

void SpeedPI_BalanceForStraight(float target_rpm)
{
    float balanced_output =
        SpeedPI_Clamp((g_left.output + g_right.output) * 0.5f,
                      SPEED_PWM_MIN, SPEED_PWM_MAX);

    g_left.target_rpm = target_rpm;
    g_right.target_rpm = target_rpm;
    g_left.actual_rpm = Encoder_GetLeftSpeed();
    g_right.actual_rpm = Encoder_GetRightSpeed();
    g_left.last_error = target_rpm - g_left.actual_rpm;
    g_right.last_error = target_rpm - g_right.actual_rpm;
    g_left.output = balanced_output;
    g_right.output = balanced_output;
    g_left.pwm = (int)(balanced_output + 0.5f);
    g_right.pwm = (int)(balanced_output + 0.5f);
    move(g_left.pwm, g_right.pwm);
}

void SpeedPI_Reset(void)
{
    SpeedPI_ResetChannel(&g_left);
    SpeedPI_ResetChannel(&g_right);
    Motor_Coast();
}

int SpeedPI_GetLeftPWM(void) { return g_left.pwm; }
int SpeedPI_GetRightPWM(void) { return g_right.pwm; }
float SpeedPI_GetLeftTarget(void) { return g_left.target_rpm; }
float SpeedPI_GetRightTarget(void) { return g_right.target_rpm; }
float SpeedPI_GetLeftRPM(void) { return g_left.actual_rpm; }
float SpeedPI_GetRightRPM(void) { return g_right.actual_rpm; }

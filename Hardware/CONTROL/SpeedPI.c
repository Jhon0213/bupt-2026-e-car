#include "SpeedPI.h"

#include "Hardware/Encoder.h"
#include "Hardware/Motor.h"

#define SPEED_LEFT_KP     4.5f
#define SPEED_LEFT_KI     0.0f
#define SPEED_LEFT_KD     0.0f
#define SPEED_RIGHT_KP    0.0f
#define SPEED_RIGHT_KI    0.0f
#define SPEED_RIGHT_KD    0.0f
#define SPEED_PWM_MIN     0.0f
#define SPEED_PWM_MAX     500.0f
#define SPEED_LEFT_START_PWM   35.0f
#define SPEED_RIGHT_START_PWM  45.0f
#define SPEED_LEFT_KEEP_PWM    35.0f
#define SPEED_RIGHT_KEEP_PWM   32.0f
#define SPEED_KEEP_PWM_RPM_THRESHOLD 5.0f
#define SPEED_ERROR_HOLD_RPM 0.8f
#define SPEED_PWM_SLEW_ERROR_RPM 2.0f
#define SPEED_PWM_SLEW_FAST_STEP 20.0f
#define SPEED_PWM_SLEW_SLOW_STEP 3.0f
#define SPEED_PWM_SLEW_DECEL_STEP 8.0f
#define SPEED_PWM_SLEW_DECEL_TICKS 30U

typedef struct
{
    float target_rpm;
    float actual_rpm;
    float last_target_rpm;
    float last_error;
    float prev_error;
    float output;
    float raw_output;
    int pwm;
    unsigned int decel_slew_ticks;
} SpeedPIChannel;

static SpeedPIChannel g_left;
static SpeedPIChannel g_right;

static float SpeedPI_Clamp(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static float SpeedPI_Abs(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float SpeedPI_ApplyPWMSlew(float previous_pwm,
                                  float requested_pwm,
                                  float error_rpm,
                                  unsigned int decel_active)
{
    float step_limit = (SpeedPI_Abs(error_rpm) > SPEED_PWM_SLEW_ERROR_RPM)
                     ? SPEED_PWM_SLEW_FAST_STEP
                     : SPEED_PWM_SLEW_SLOW_STEP;
    float delta = requested_pwm - previous_pwm;

    if ((decel_active != 0U) && (delta < 0.0f) &&
        (error_rpm < -SPEED_ERROR_HOLD_RPM) &&
        (step_limit < SPEED_PWM_SLEW_DECEL_STEP))
    {
        step_limit = SPEED_PWM_SLEW_DECEL_STEP;
    }

    if (delta > step_limit)
    {
        return previous_pwm + step_limit;
    }
    if (delta < -step_limit)
    {
        return previous_pwm - step_limit;
    }
    return requested_pwm;
}

static float SpeedPI_ApplyDeadzonePWM(float pwm, float target_rpm,
                                      float actual_rpm,
                                      float start_pwm,
                                      float keep_pwm)
{
    float minimum_pwm;

    if ((target_rpm <= 0.0f) || (pwm <= 0.0f))
    {
        return pwm;
    }

    minimum_pwm = (actual_rpm > SPEED_KEEP_PWM_RPM_THRESHOLD) ? keep_pwm : start_pwm;

    if (pwm < minimum_pwm)
    {
        return minimum_pwm;
    }
    return pwm;
}

static void SpeedPI_ResetChannel(SpeedPIChannel *channel)
{
    channel->target_rpm = 0.0f;
    channel->actual_rpm = 0.0f;
    channel->last_target_rpm = 0.0f;
    channel->last_error = 0.0f;
    channel->prev_error = 0.0f;
    channel->output = 0.0f;
    channel->raw_output = 0.0f;
    channel->pwm = 0;
    channel->decel_slew_ticks = 0U;
}

static void SpeedPI_UpdateChannel(SpeedPIChannel *channel,
                                  float target_rpm,
                                  float actual_rpm,
                                  float kp,
                                  float ki,
                                  float kd,
                                  float start_pwm,
                                  float keep_pwm)
{
    float error;
    float increment;
    float raw_output;

    if (target_rpm < (channel->target_rpm - SPEED_ERROR_HOLD_RPM))
    {
        channel->decel_slew_ticks = SPEED_PWM_SLEW_DECEL_TICKS;
    }
    else if (channel->decel_slew_ticks > 0U)
    {
        channel->decel_slew_ticks--;
    }

    channel->last_target_rpm = channel->target_rpm;
    channel->target_rpm = target_rpm;
    channel->actual_rpm = actual_rpm;

    if (target_rpm <= 0.0f)
    {
        channel->last_error = 0.0f;
        channel->prev_error = 0.0f;
        channel->output = 0.0f;
        channel->raw_output = 0.0f;
        channel->pwm = 0;
        channel->decel_slew_ticks = 0U;
        return;
    }

    error = target_rpm - actual_rpm;
    if (SpeedPI_Abs(error) < SPEED_ERROR_HOLD_RPM)
    {
        channel->raw_output = channel->output;
        channel->prev_error = channel->last_error;
        channel->last_error = error;
        return;
    }
    increment = kp * (error - channel->last_error)
              + ki * error
              + kd * (error - 2.0f * channel->last_error
                    + channel->prev_error);
    raw_output = SpeedPI_Clamp(channel->output + increment,
                               SPEED_PWM_MIN,
                               SPEED_PWM_MAX);
    channel->raw_output = raw_output;
    raw_output = SpeedPI_ApplyDeadzonePWM(raw_output,
                                          target_rpm,
                                          actual_rpm,
                                          start_pwm,
                                          keep_pwm);
    channel->output = SpeedPI_ApplyPWMSlew(channel->output,
                                           raw_output,
                                           error,
                                           channel->decel_slew_ticks);
    channel->prev_error = channel->last_error;
    channel->last_error = error;
    channel->pwm = (int)(channel->output + 0.5f);
}

void SpeedPI_Init(void)
{
    SpeedPI_Reset();
}

void SpeedPI_Update(float left_target_rpm, float right_target_rpm)
{
    SpeedPI_UpdateChannel(&g_left, left_target_rpm, Encoder_GetLeftSpeed(),
                          SPEED_LEFT_KP, SPEED_LEFT_KI, SPEED_LEFT_KD,
                          SPEED_LEFT_START_PWM, SPEED_LEFT_KEEP_PWM);
    SpeedPI_UpdateChannel(&g_right, right_target_rpm, Encoder_GetRightSpeed(),
                          SPEED_RIGHT_KP, SPEED_RIGHT_KI, SPEED_RIGHT_KD,
                          SPEED_RIGHT_START_PWM, SPEED_RIGHT_KEEP_PWM);
    move(g_left.pwm, g_right.pwm);
}

void SpeedPI_UpdateLeftOnly(float target_rpm)
{
    SpeedPI_UpdateChannel(&g_left, target_rpm, Encoder_GetLeftSpeed(),
                          SPEED_LEFT_KP, SPEED_LEFT_KI, SPEED_LEFT_KD,
                          SPEED_LEFT_START_PWM, SPEED_LEFT_KEEP_PWM);
    move(g_left.pwm, 0);
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
    g_left.prev_error = g_left.last_error;
    g_right.prev_error = g_right.last_error;
    g_left.last_error = target_rpm - g_left.actual_rpm;
    g_right.last_error = target_rpm - g_right.actual_rpm;
    g_left.raw_output = balanced_output;
    g_right.raw_output = balanced_output;
    g_left.output = SpeedPI_ApplyDeadzonePWM(balanced_output, target_rpm,
                                             g_left.actual_rpm,
                                             SPEED_LEFT_START_PWM,
                                             SPEED_LEFT_KEEP_PWM);
    g_right.output = SpeedPI_ApplyDeadzonePWM(balanced_output, target_rpm,
                                              g_right.actual_rpm,
                                              SPEED_RIGHT_START_PWM,
                                              SPEED_RIGHT_KEEP_PWM);
    g_left.pwm = (int)(g_left.output + 0.5f);
    g_right.pwm = (int)(g_right.output + 0.5f);
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
float SpeedPI_GetLeftRawPWM(void) { return g_left.raw_output; }
float SpeedPI_GetRightRawPWM(void) { return g_right.raw_output; }
float SpeedPI_GetLeftTarget(void) { return g_left.target_rpm; }
float SpeedPI_GetRightTarget(void) { return g_right.target_rpm; }
float SpeedPI_GetLeftRPM(void) { return g_left.actual_rpm; }
float SpeedPI_GetRightRPM(void) { return g_right.actual_rpm; }

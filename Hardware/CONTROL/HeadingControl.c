#include "HeadingControl.h"

#define HEADING_KP_DEFAULT             1.0f
#define HEADING_KI_DEFAULT             0.0f
#define HEADING_KD_DEFAULT             0.0f
#define HEADING_INTEGRAL_LIMIT       100.0f
#define HEADING_CORRECTION_LIMIT_RPM  60.0f
#define HEADING_TARGET_MIN_RPM         0.0f
#define HEADING_TARGET_MAX_RPM       180.0f

static float g_kp;
static float g_ki;
static float g_kd;
static float g_last_error;
static HeadingControl_Output g_output;

static float Clamp(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static void UpdateTargets(void)
{
    g_output.left_target_rpm =
        Clamp(g_output.base_rpm + g_output.correction_rpm,
              HEADING_TARGET_MIN_RPM,
              HEADING_TARGET_MAX_RPM);
    g_output.right_target_rpm =
        Clamp(g_output.base_rpm - g_output.correction_rpm,
              HEADING_TARGET_MIN_RPM,
              HEADING_TARGET_MAX_RPM);
}

void HeadingControl_Init(void)
{
    g_kp = HEADING_KP_DEFAULT;
    g_ki = HEADING_KI_DEFAULT;
    g_kd = HEADING_KD_DEFAULT;
    g_output.target_yaw_deg = 0.0f;
    g_output.base_rpm = 0.0f;
    HeadingControl_Reset(0.0f);
}

void HeadingControl_Reset(float actual_yaw_deg)
{
    g_output.actual_yaw_deg = actual_yaw_deg;
    g_output.error_deg = g_output.target_yaw_deg - actual_yaw_deg;
    g_output.error_integral = 0.0f;
    g_output.error_derivative = 0.0f;
    g_output.correction_rpm = 0.0f;
    g_last_error = g_output.error_deg;
    UpdateTargets();
}

void HeadingControl_SetGains(float kp, float ki, float kd)
{
    g_kp = kp;
    g_ki = ki;
    g_kd = kd;
}

void HeadingControl_SetTarget(float target_yaw_deg, float base_rpm)
{
    g_output.target_yaw_deg = target_yaw_deg;
    g_output.base_rpm = base_rpm;
    UpdateTargets();
}

void HeadingControl_SetBaseRPM(float base_rpm)
{
    g_output.base_rpm = base_rpm;
    UpdateTargets();
}

void HeadingControl_Update(float actual_yaw_deg)
{
    float pid_output;

    g_output.actual_yaw_deg = actual_yaw_deg;
    g_output.error_deg = g_output.target_yaw_deg - actual_yaw_deg;
    g_output.error_integral =
        Clamp(g_output.error_integral + g_output.error_deg,
              -HEADING_INTEGRAL_LIMIT,
              HEADING_INTEGRAL_LIMIT);
    g_output.error_derivative = g_output.error_deg - g_last_error;

    pid_output = g_kp * g_output.error_deg
               + g_ki * g_output.error_integral
               + g_kd * g_output.error_derivative;
    g_output.correction_rpm =
        Clamp(pid_output,
              -HEADING_CORRECTION_LIMIT_RPM,
              HEADING_CORRECTION_LIMIT_RPM);

    g_last_error = g_output.error_deg;
    UpdateTargets();
}

void HeadingControl_GetOutput(HeadingControl_Output *output)
{
    if (output != 0)
    {
        *output = g_output;
    }
}
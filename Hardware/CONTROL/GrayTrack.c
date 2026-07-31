#include "GrayTrack.h"

#include "Hardware/Gray.h"

#define GRAY_BASE_RPM_DEFAULT          100.0f
#define GRAY_KP_RPM_DEFAULT             8.0f
#define GRAY_KD_RPM_DEFAULT             0.0f
#define GRAY_CORRECTION_MAX_DEFAULT     45.0f
#define GRAY_CORRECTION_RISE_STEP_DEFAULT 8.0f
#define GRAY_CORRECTION_FALL_STEP_DEFAULT 10.0f
#define GRAY_TARGET_MIN         50.0f
#define GRAY_TARGET_MAX        145.0f
#define GRAY_LOST_CONFIRM_COUNT 12U
#define GRAY_ERROR_DEADBAND_DEFAULT 1U
#define GRAY_ERROR_STEP_MAX_DEFAULT 1U
#define GRAY_TRACK_CHANNELS     6U
#define GRAY_TRACK_MASK         0x3FU

static uint8_t g_raw;
static uint8_t g_black_mask;
static int16_t g_error;
static int16_t g_last_error;
static uint8_t g_lost;
static uint8_t g_lost_count;
static uint8_t g_lost_confirmed;
static uint8_t g_error_deadband = GRAY_ERROR_DEADBAND_DEFAULT;
static uint8_t g_error_step_max = GRAY_ERROR_STEP_MAX_DEFAULT;
static float g_base_rpm = GRAY_BASE_RPM_DEFAULT;
static float g_kp_rpm = GRAY_KP_RPM_DEFAULT;
static float g_kd_rpm = GRAY_KD_RPM_DEFAULT;
static float g_correction_max_rpm = GRAY_CORRECTION_MAX_DEFAULT;
static float g_correction_rise_step = GRAY_CORRECTION_RISE_STEP_DEFAULT;
static float g_correction_fall_step = GRAY_CORRECTION_FALL_STEP_DEFAULT;
static float g_correction_rpm;
static float g_left_target_rpm;
static float g_right_target_rpm;

static float GrayTrack_Clamp(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static int16_t GrayTrack_AbsInt16(int16_t value)
{
    return (value < 0) ? (int16_t)(-value) : value;
}

static float GrayTrack_AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float GrayTrack_SlewCorrection(float current, float target)
{
    float delta;
    float step;

    target = GrayTrack_Clamp(target,
                             -g_correction_max_rpm,
                             g_correction_max_rpm);
    delta = target - current;
    step = (GrayTrack_AbsFloat(target) > GrayTrack_AbsFloat(current)) ?
        g_correction_rise_step :
        g_correction_fall_step;

    if (delta > step)
    {
        delta = step;
    }
    else if (delta < -step)
    {
        delta = -step;
    }

    return GrayTrack_Clamp(current + delta,
                           -g_correction_max_rpm,
                           g_correction_max_rpm);
}

static int16_t GrayTrack_LimitErrorStep(int16_t target_error)
{
    int16_t delta = (int16_t)(target_error - g_last_error);
    int16_t step_max = (int16_t)g_error_step_max;

    if (delta > step_max)
    {
        return (int16_t)(g_last_error + step_max);
    }
    if (delta < (int16_t)(-step_max))
    {
        return (int16_t)(g_last_error - step_max);
    }
    return target_error;
}

static int16_t GrayTrack_CalculateTrackedError(uint8_t black_mask,
                                                int16_t reference_error)
{
    static const int8_t weights[GRAY_TRACK_CHANNELS] = {-5, -3, -1, 1, 3, 5};
    int16_t best_center = reference_error;
    int16_t best_distance = 32767;
    uint8_t best_count = 0U;
    uint8_t i = 0U;

    black_mask &= GRAY_TRACK_MASK;

    while (i < GRAY_TRACK_CHANNELS)
    {
        int16_t sum = 0;
        uint8_t count = 0U;
        int16_t center;
        int16_t distance;

        while ((i < GRAY_TRACK_CHANNELS) &&
               ((black_mask & (uint8_t)(1U << i)) == 0U))
        {
            i++;
        }

        while ((i < GRAY_TRACK_CHANNELS) &&
               ((black_mask & (uint8_t)(1U << i)) != 0U))
        {
            sum += weights[i];
            count++;
            i++;
        }

        if (count == 0U)
        {
            continue;
        }

        center = (int16_t)(sum / (int16_t)count);
        distance = GrayTrack_AbsInt16((int16_t)(center - reference_error));

        if ((distance < best_distance) ||
            ((distance == best_distance) && (count > best_count)))
        {
            best_center = center;
            best_distance = distance;
            best_count = count;
        }
    }

    return best_center;
}

void GrayTrack_SetErrorDeadband(uint8_t deadband)
{
    g_error_deadband = deadband;
}

void GrayTrack_SetParams(float base_rpm,
                         float kp_rpm,
                         float correction_max_rpm,
                         float correction_rise_step,
                         float correction_fall_step)
{
    if (base_rpm < 0.0f)
    {
        base_rpm = 0.0f;
    }
    if (correction_max_rpm < 0.0f)
    {
        correction_max_rpm = -correction_max_rpm;
    }
    if (correction_rise_step < 0.1f)
    {
        correction_rise_step = 0.1f;
    }
    if (correction_fall_step < 0.1f)
    {
        correction_fall_step = 0.1f;
    }

    g_base_rpm = base_rpm;
    g_kp_rpm = kp_rpm;
    g_kd_rpm = GRAY_KD_RPM_DEFAULT;
    g_correction_max_rpm = correction_max_rpm;
    g_correction_rise_step = correction_rise_step;
    g_correction_fall_step = correction_fall_step;
    g_correction_rpm = GrayTrack_Clamp(g_correction_rpm,
                                       -g_correction_max_rpm,
                                       g_correction_max_rpm);
}

void GrayTrack_Reset(void)
{
    g_raw = 0U;
    g_black_mask = 0U;
    g_error = 0;
    g_last_error = 0;
    g_lost = 1U;
    g_lost_count = 0U;
    g_lost_confirmed = 0U;
    g_correction_rpm = 0.0f;
    g_left_target_rpm = g_base_rpm;
    g_right_target_rpm = g_base_rpm;
}

void GrayTrack_Init(void)
{
    g_base_rpm = GRAY_BASE_RPM_DEFAULT;
    g_kp_rpm = GRAY_KP_RPM_DEFAULT;
    g_kd_rpm = GRAY_KD_RPM_DEFAULT;
    g_correction_max_rpm = GRAY_CORRECTION_MAX_DEFAULT;
    g_correction_rise_step = GRAY_CORRECTION_RISE_STEP_DEFAULT;
    g_correction_fall_step = GRAY_CORRECTION_FALL_STEP_DEFAULT;
    g_error_step_max = GRAY_ERROR_STEP_MAX_DEFAULT;
    GrayTrack_Reset();
    g_error_deadband = GRAY_ERROR_DEADBAND_DEFAULT;
}

void GrayTrack_Update(void)
{
    float derivative;
    int16_t measured_error;

    Gray_Update();
    g_raw = Gray_GetRaw();
    g_black_mask = Gray_GetBlackMask();
    measured_error = GrayTrack_CalculateTrackedError(g_black_mask, g_last_error);
    if (GrayTrack_AbsInt16(measured_error) <= (int16_t)g_error_deadband)
    {
        measured_error = 0;
    }
    g_lost = Gray_IsLost();

    if (g_lost != 0U)
    {
        if (g_lost_count < GRAY_LOST_CONFIRM_COUNT)
        {
            g_lost_count++;
        }
        if (g_lost_count >= GRAY_LOST_CONFIRM_COUNT)
        {
            g_lost_confirmed = 1U;
        }
        return;
    }

    g_lost_count = 0U;
    g_lost_confirmed = 0U;
    g_error = GrayTrack_LimitErrorStep(measured_error);
    derivative = (float)(g_error - g_last_error);
    g_correction_rpm = GrayTrack_SlewCorrection(
        g_correction_rpm,
        g_kp_rpm * (float)g_error + g_kd_rpm * derivative);

    /* error<0 means the line is left: slow left wheel, speed right wheel. */
    g_left_target_rpm = GrayTrack_Clamp(g_base_rpm + g_correction_rpm,
                                        GRAY_TARGET_MIN,
                                        GRAY_TARGET_MAX);
    g_right_target_rpm = GrayTrack_Clamp(g_base_rpm - g_correction_rpm,
                                         GRAY_TARGET_MIN,
                                         GRAY_TARGET_MAX);
    g_last_error = g_error;
}

uint8_t GrayTrack_GetRaw(void) { return g_raw; }
uint8_t GrayTrack_GetBlackMask(void) { return g_black_mask; }
int16_t GrayTrack_GetError(void) { return g_error; }
uint8_t GrayTrack_IsLost(void) { return g_lost; }
uint8_t GrayTrack_IsLostConfirmed(void) { return g_lost_confirmed; }
float GrayTrack_GetCorrectionRPM(void) { return g_correction_rpm; }
float GrayTrack_GetLeftTargetRPM(void) { return g_left_target_rpm; }
float GrayTrack_GetRightTargetRPM(void) { return g_right_target_rpm; }
float GrayTrack_GetBaseRPM(void) { return g_base_rpm; }
float GrayTrack_GetKpRPM(void) { return g_kp_rpm; }
float GrayTrack_GetKdRPM(void) { return g_kd_rpm; }
float GrayTrack_GetCorrectionMaxRPM(void) { return g_correction_max_rpm; }
float GrayTrack_GetTargetMinRPM(void) { return GRAY_TARGET_MIN; }
float GrayTrack_GetTargetMaxRPM(void) { return GRAY_TARGET_MAX; }

void GrayTrack_GetOutput(GrayTrack_Output *output)
{
    if (output == 0)
    {
        return;
    }

    output->raw = g_raw;
    output->black_mask = g_black_mask;
    output->error = g_error;
    output->line_detected = (g_lost == 0U) ? 1U : 0U;
    output->lost_confirmed = g_lost_confirmed;
    output->correction_rpm = g_correction_rpm;
    output->left_target_rpm = g_left_target_rpm;
    output->right_target_rpm = g_right_target_rpm;
}

uint8_t GrayTrack_IsValid(void)
{
    return (g_lost == 0U) ? 1U : 0U;
}







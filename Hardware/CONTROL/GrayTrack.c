#include "GrayTrack.h"

#include "Hardware/Gray.h"

#define GRAY_BASE_RPM           80.0f
#define GRAY_KP_RPM             4.0f
#define GRAY_KD_RPM            0.0f
#define GRAY_CORRECTION_MAX     30.0f
#define GRAY_TARGET_MIN         50.0f
#define GRAY_TARGET_MAX        115.0f
#define GRAY_LOST_CONFIRM_COUNT 3U

static uint8_t g_raw;
static uint8_t g_black_mask;
static int16_t g_error;
static int16_t g_last_error;
static uint8_t g_lost;
static uint8_t g_lost_count;
static uint8_t g_lost_confirmed;
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

static int16_t GrayTrack_CalculateTrackedError(uint8_t black_mask,
                                                int16_t reference_error)
{
    static const int8_t weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};
    int16_t best_center = reference_error;
    int16_t best_distance = 32767;
    uint8_t best_count = 0U;
    uint8_t i = 0U;

    while (i < 8U)
    {
        int16_t sum = 0;
        uint8_t count = 0U;
        int16_t center;
        int16_t distance;

        while ((i < 8U) &&
               ((black_mask & (uint8_t)(1U << i)) == 0U))
        {
            i++;
        }

        while ((i < 8U) &&
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
void GrayTrack_Reset(void)
{
    g_raw = 0xFFU;
    g_black_mask = 0U;
    g_error = 0;
    g_last_error = 0;
    g_lost = 1U;
    g_lost_count = 0U;
    g_lost_confirmed = 0U;
    g_correction_rpm = 0.0f;
    g_left_target_rpm = GRAY_BASE_RPM;
    g_right_target_rpm = GRAY_BASE_RPM;
}

void GrayTrack_Init(void)
{
    GrayTrack_Reset();
}

void GrayTrack_Update(void)
{
    float derivative;

    Gray_Update();
    g_raw = Gray_GetRaw();
    g_black_mask = Gray_GetBlackMask();
    g_error = GrayTrack_CalculateTrackedError(g_black_mask, g_last_error);
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
    derivative = (float)(g_error - g_last_error);
    g_correction_rpm = GRAY_KP_RPM * (float)g_error
                     + GRAY_KD_RPM * derivative;
    g_correction_rpm = GrayTrack_Clamp(g_correction_rpm,
                                       -GRAY_CORRECTION_MAX,
                                       GRAY_CORRECTION_MAX);

    /* error<0 means the line is left: slow left wheel, speed right wheel. */
    g_left_target_rpm = GrayTrack_Clamp(GRAY_BASE_RPM + g_correction_rpm,
                                        GRAY_TARGET_MIN,
                                        GRAY_TARGET_MAX);
    g_right_target_rpm = GrayTrack_Clamp(GRAY_BASE_RPM - g_correction_rpm,
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
float GrayTrack_GetBaseRPM(void) { return GRAY_BASE_RPM; }
float GrayTrack_GetKpRPM(void) { return GRAY_KP_RPM; }
float GrayTrack_GetKdRPM(void) { return GRAY_KD_RPM; }
float GrayTrack_GetCorrectionMaxRPM(void) { return GRAY_CORRECTION_MAX; }
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

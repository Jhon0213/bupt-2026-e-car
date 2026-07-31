#include <stdint.h>

#include "Application/Task3_LinkedOperation.h"

#include "Application/RouteNavigator.h"
#include "Hardware/Diagnostics/ControlTimingDiag.h"
#include "Hardware/Encoder.h"
#include "Hardware/Gray.h"
#include "Hardware/Motor.h"
#include "Hardware/StarFlash.h"
#include "Hardware/CONTROL/GrayTrack.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Public/Board/board.h"

#define TASK3_CONTROL_MS ROUTE_NAVIGATOR_CONTROL_MS
#define TASK3_DEBUG_STREAM 1U
#define TASK3_DEBUG_LOG_MS 50U
#define TASK3_STRAIGHT_DIAG_MODE 0U
#define TASK3_STRAIGHT_DIAG_MS 1000U
#define TASK3_STRAIGHT_DIAG_TARGET_RPM 100.0f
#define TASK3_CURVE_LOST_HOLD_MS 800U
#define TASK3_CURVE_LOST_HOLD_SCALE 0.8f
#define TASK3_STRAIGHT_TRANSITION_MS 500U
#define TASK3_NORMAL_ERROR_DEADBAND 1U
#define TASK3_PRE_CURVE_START_COUNT 82300L
#define TASK3_PRE_CURVE_RAMP_COUNT 10300L
#define TASK3_STRAIGHT_BASE_RPM 100.0f
#define TASK3_STRAIGHT_KP_RPM 5.5f
#define TASK3_STRAIGHT_CORRECTION_MAX 45.0f
#define TASK3_STRAIGHT_RISE_STEP 4.0f
#define TASK3_STRAIGHT_FALL_STEP 10.0f
#define TASK3_CURVE_BASE_RPM 88.0f
#define TASK3_CURVE_KP_RPM 5.0f
#define TASK3_CURVE_CORRECTION_MAX 42.0f
#define TASK3_CURVE_RISE_STEP 3.0f
#define TASK3_CURVE_FALL_STEP 4.0f
#define TASK3_CURVE_RAMP_DEG 45.0f
#define TASK3_BC_EXIT_COMP_CM 25.0f
#define TASK3_BC_EXIT_COMP_RPM 9.0f
#define TASK3_STOP_AT_BC_EXIT_COMP_START 0U
#define TASK3_POINT_STOP_MS 10000U
#define TASK3_POINT_STOP_ENABLE 0U
#define TASK3_B_PLUS_STOP_AFTER_B_CM 5.0f
#define TASK3_TARGET_RPM_MAX 145.0f
#define TASK3_PRE_CURVE_BASE_RPM 92.0f
#define TASK3_PRE_CURVE_KP_RPM 5.8f
#define TASK3_PRE_CURVE_CORRECTION_MAX 42.0f
#define TASK3_PRE_CURVE_RISE_STEP 5.0f
#define TASK3_PRE_CURVE_FALL_STEP 7.0f
#define TASK3_PARAM_BASE_STEP 0.2f
#define TASK3_PARAM_KP_STEP 0.04f
#define TASK3_PARAM_CORRECTION_MAX_STEP 0.4f
#define TASK3_PARAM_SHAPE_STEP 0.2f
#define TASK3_PI 3.1415926f
#define TASK3_WHEEL_DIAMETER_CM 6.5f
#define TASK3_TRACK_STRAIGHT_CM 150.0f
#define TASK3_TRACK_RADIUS_CM 50.0f
#define TASK3_FINISH_WINDOW_BEFORE_CM 60.0f
#define TASK3_FINISH_STOP_OFFSET_CM 33.0f
#define TASK3_FINISH_CONFIRM_COUNT 1U

typedef enum
{
    TASK3_SEG_AB = 0,
    TASK3_SEG_BC,
    TASK3_SEG_CD,
    TASK3_SEG_DA
} Task3Segment;

static uint8_t g_task3_running;
static uint32_t g_task3_next_control_ms;
static uint32_t g_task3_start_ms;
static GrayTrack_Output g_task3_gray;
static uint8_t g_task3_has_valid_line;
static float g_task3_last_valid_left_rpm;
static float g_task3_last_valid_right_rpm;
static uint8_t g_task3_curve_lost_hold;
static uint32_t g_task3_curve_lost_hold_ms;
static uint8_t g_task3_straight_transition;
static uint32_t g_task3_straight_transition_ms;
static float g_task3_progress_deg;
static uint8_t g_task3_progress_ready;
static Task3Segment g_task3_segment;
static Task3Segment g_task3_last_sent_segment;
static uint8_t g_task3_segment_sent;
static int32_t g_task3_lap_left_base_count;
static int32_t g_task3_lap_right_base_count;
static int32_t g_task3_lap_advance_count;
static int32_t g_task3_segment_left_base_count;
static int32_t g_task3_segment_right_base_count;
static int32_t g_task3_segment_advance_count;
static uint8_t g_task3_finish_window;
static uint8_t g_task3_finish_confirm_count;
static uint8_t g_task3_pre_curve;
static float g_task3_curve_entry_ramp;
static uint8_t g_task3_gray_params_ready;
static float g_task3_gray_base_rpm;
static float g_task3_gray_kp_rpm;
static float g_task3_gray_correction_max;
static float g_task3_gray_rise_step;
static float g_task3_gray_fall_step;
static uint32_t g_task3_next_debug_ms;
static uint8_t g_task3_debug_stream_enabled = 1U;
static uint8_t g_task3_debug_mode;
static uint8_t g_task3_lap_done;
static Task3_RunMode g_task3_run_mode;
static uint8_t g_task3_point_stop_active;
static uint32_t g_task3_point_stop_until_ms;
static Task3Segment g_task3_point_stop_segment;
static uint8_t g_task3_point_stop_mask;

static const char *Task3_SegmentCode(Task3Segment segment)
{
    switch (segment)
    {
        case TASK3_SEG_BC:
            return "BC";
        case TASK3_SEG_CD:
            return "CD";
        case TASK3_SEG_DA:
            return "DA";
        case TASK3_SEG_AB:
        default:
            return "AB";
    }
}

static uint8_t Task3_IsCurveSegment(void)
{
    return ((g_task3_segment == TASK3_SEG_BC) ||
            (g_task3_segment == TASK3_SEG_DA)) ? 1U : 0U;
}

static uint8_t Task3_IsLostHoldAllowed(void)
{
    return ((Task3_IsCurveSegment() != 0U) ||
            (g_task3_straight_transition != 0U) ||
            (g_task3_pre_curve != 0U)) ? 1U : 0U;
}

static float Task3_ClampFloat(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static float Task3_LerpFloat(float start, float end, float factor)
{
    factor = Task3_ClampFloat(factor, 0.0f, 1.0f);
    return start + ((end - start) * factor);
}

static float Task3_SlewFloat(float current, float target, float step)
{
    float delta = target - current;

    if (delta > step)
    {
        return current + step;
    }
    if (delta < -step)
    {
        return current - step;
    }
    return target;
}

static void Task3_SetGrayParamsSmooth(uint8_t deadband,
                                      float base_rpm,
                                      float kp_rpm,
                                      float correction_max,
                                      float rise_step,
                                      float fall_step)
{
    GrayTrack_SetErrorDeadband(deadband);

    if (g_task3_gray_params_ready == 0U)
    {
        g_task3_gray_base_rpm = base_rpm;
        g_task3_gray_kp_rpm = kp_rpm;
        g_task3_gray_correction_max = correction_max;
        g_task3_gray_rise_step = rise_step;
        g_task3_gray_fall_step = fall_step;
        g_task3_gray_params_ready = 1U;
    }
    else
    {
        g_task3_gray_base_rpm = Task3_SlewFloat(g_task3_gray_base_rpm,
                                                base_rpm,
                                                TASK3_PARAM_BASE_STEP);
        g_task3_gray_kp_rpm = Task3_SlewFloat(g_task3_gray_kp_rpm,
                                              kp_rpm,
                                              TASK3_PARAM_KP_STEP);
        g_task3_gray_correction_max = Task3_SlewFloat(g_task3_gray_correction_max,
                                                      correction_max,
                                                      TASK3_PARAM_CORRECTION_MAX_STEP);
        g_task3_gray_rise_step = Task3_SlewFloat(g_task3_gray_rise_step,
                                                 rise_step,
                                                 TASK3_PARAM_SHAPE_STEP);
        g_task3_gray_fall_step = Task3_SlewFloat(g_task3_gray_fall_step,
                                                 fall_step,
                                                 TASK3_PARAM_SHAPE_STEP);
    }

    GrayTrack_SetParams(g_task3_gray_base_rpm,
                        g_task3_gray_kp_rpm,
                        g_task3_gray_correction_max,
                        g_task3_gray_rise_step,
                        g_task3_gray_fall_step);
}

static int32_t Task3_AbsInt32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int32_t Task3_DistanceCmToCount(float distance_cm)
{
    float wheel_cm = TASK3_WHEEL_DIAMETER_CM * TASK3_PI;
    float count = distance_cm * PULSE_PER_CYCLE / wheel_cm;

    return (int32_t)(count + 0.5f);
}

static float Task3_GetLapDistanceCm(void)
{
    return (2.0f * TASK3_TRACK_STRAIGHT_CM) +
           (2.0f * TASK3_PI * TASK3_TRACK_RADIUS_CM);
}

static int32_t Task3_GetLapTargetCount(void)
{
    return Task3_DistanceCmToCount(Task3_GetLapDistanceCm());
}

static int32_t Task3_GetStraightTargetCount(void)
{
    return Task3_DistanceCmToCount(TASK3_TRACK_STRAIGHT_CM);
}

static int32_t Task3_GetCurveTargetCount(void)
{
    return Task3_DistanceCmToCount(TASK3_PI * TASK3_TRACK_RADIUS_CM);
}

static int32_t Task3_GetCurveRampCount(void)
{
    float ramp_cm = TASK3_PI * TASK3_TRACK_RADIUS_CM *
                    (TASK3_CURVE_RAMP_DEG / 180.0f);
    return Task3_DistanceCmToCount(ramp_cm);
}

static Task3Segment Task3_GetSegmentByLapCount(int32_t lap_count)
{
    int32_t ab_end_count = Task3_GetStraightTargetCount();
    int32_t bc_end_count = ab_end_count + Task3_GetCurveTargetCount();
    int32_t cd_end_count = bc_end_count + Task3_GetStraightTargetCount();

    if (lap_count < ab_end_count)
    {
        return TASK3_SEG_AB;
    }
    if (lap_count < bc_end_count)
    {
        return TASK3_SEG_BC;
    }
    if (lap_count < cd_end_count)
    {
        return TASK3_SEG_CD;
    }
    return TASK3_SEG_DA;
}

static int32_t Task3_GetSegmentAdvanceCount(void)
{
    int32_t left_delta = Encoder_GetLeftCount() - g_task3_segment_left_base_count;
    int32_t right_delta = Encoder_GetRightCount() - g_task3_segment_right_base_count;

    return (Task3_AbsInt32(left_delta) + Task3_AbsInt32(right_delta)) / 2;
}

static int32_t Task3_GetLapAdvanceCount(void)
{
    int32_t left_delta = Encoder_GetLeftCount() - g_task3_lap_left_base_count;
    int32_t right_delta = Encoder_GetRightCount() - g_task3_lap_right_base_count;

    return (Task3_AbsInt32(left_delta) + Task3_AbsInt32(right_delta)) / 2;
}

static void Task3_RecordLapEncoderStart(void)
{
    g_task3_lap_left_base_count = Encoder_GetLeftCount();
    g_task3_lap_right_base_count = Encoder_GetRightCount();
    g_task3_lap_advance_count = 0;
    g_task3_finish_window = 0U;
    g_task3_finish_confirm_count = 0U;
}

static void Task3_RecordSegmentEncoderStart(void)
{
    g_task3_segment_left_base_count = Encoder_GetLeftCount();
    g_task3_segment_right_base_count = Encoder_GetRightCount();
    g_task3_segment_advance_count = 0;
    g_task3_pre_curve = 0U;
}

static uint8_t Task3_IsStraightSegment(void)
{
    return ((g_task3_segment == TASK3_SEG_AB) ||
            (g_task3_segment == TASK3_SEG_CD)) ? 1U : 0U;
}

static float Task3_GetPreCurveRampFactor(void)
{
    float ramp_count;

    if (g_task3_pre_curve == 0U)
    {
        return 0.0f;
    }

    ramp_count = (float)(g_task3_segment_advance_count -
                         TASK3_PRE_CURVE_START_COUNT);
    return Task3_ClampFloat(ramp_count /
                            (float)TASK3_PRE_CURVE_RAMP_COUNT,
                            0.0f,
                            1.0f);
}

static float Task3_GetCurveRampFactor(void)
{
    int32_t ramp_count;
    float ramp;

    if (Task3_IsCurveSegment() == 0U)
    {
        return 0.0f;
    }

    ramp_count = Task3_GetCurveRampCount();
    if (ramp_count <= 0)
    {
        return 1.0f;
    }

    ramp = Task3_ClampFloat((float)g_task3_segment_advance_count /
                            (float)ramp_count,
                            0.0f,
                            1.0f);
    if (ramp < g_task3_curve_entry_ramp)
    {
        ramp = g_task3_curve_entry_ramp;
    }
    return ramp;
}

static uint8_t Task3_ShouldStopAtBcExitCompStart(void)
{
#if TASK3_STOP_AT_BC_EXIT_COMP_START
    return ((g_task3_segment == TASK3_SEG_CD) &&
            (g_task3_segment_advance_count == 0)) ? 1U : 0U;
#else
    return 0U;
#endif
}
static void Task3_ApplyBcExitCompensation(void)
{
    int32_t compensation_count;
    float factor;
    float compensation_rpm;

    if (g_task3_segment != TASK3_SEG_CD)
    {
        return;
    }

    compensation_count = Task3_DistanceCmToCount(TASK3_BC_EXIT_COMP_CM);
    if ((compensation_count <= 0) ||
        (g_task3_segment_advance_count >= compensation_count))
    {
        return;
    }

    factor = 1.0f - ((float)g_task3_segment_advance_count /
                     (float)compensation_count);
    factor = Task3_ClampFloat(factor, 0.0f, 1.0f);
    compensation_rpm = TASK3_BC_EXIT_COMP_RPM * factor;

    g_task3_gray.left_target_rpm = Task3_ClampFloat(
        g_task3_gray.left_target_rpm + compensation_rpm,
        0.0f,
        TASK3_TARGET_RPM_MAX);
    g_task3_gray.right_target_rpm = Task3_ClampFloat(
        g_task3_gray.right_target_rpm - compensation_rpm,
        0.0f,
        TASK3_TARGET_RPM_MAX);
}
static void Task3_UpdatePreCurveByEncoder(void)
{
    g_task3_lap_advance_count = Task3_GetLapAdvanceCount();
    g_task3_segment_advance_count = Task3_GetSegmentAdvanceCount();

    if (Task3_IsStraightSegment() == 0U)
    {
        g_task3_pre_curve = 0U;
        return;
    }

    if (g_task3_segment_advance_count >= TASK3_PRE_CURVE_START_COUNT)
    {
        g_task3_pre_curve = 1U;
    }
    else
    {
        g_task3_pre_curve = 0U;
    }
}

static void Task3_StartStraightTransition(void)
{
    g_task3_straight_transition = 1U;
    g_task3_straight_transition_ms = 0U;
}

static void Task3_UpdateStraightTransitionTimer(void)
{
    if (g_task3_straight_transition == 0U)
    {
        return;
    }

    if (g_task3_straight_transition_ms < 0xFFFFFFFFU - TASK3_CONTROL_MS)
    {
        g_task3_straight_transition_ms += TASK3_CONTROL_MS;
    }

    if (g_task3_straight_transition_ms >= TASK3_STRAIGHT_TRANSITION_MS)
    {
        g_task3_straight_transition = 0U;
        g_task3_straight_transition_ms = 0U;
    }
}

static void Task3_SendCurrentSegment(void)
{
#if TASK3_DEBUG_STREAM
    g_task3_last_sent_segment = g_task3_segment;
    g_task3_segment_sent = 1U;
#else
    StarFlash_SendString("SEG,");
    StarFlash_SendString(Task3_SegmentCode(g_task3_segment));
    StarFlash_SendString("\r\n");
    g_task3_last_sent_segment = g_task3_segment;
    g_task3_segment_sent = 1U;
#endif
}

static void Task3_SendSegmentIfChanged(Task3Segment previous_segment)
{
    if (g_task3_progress_ready == 0U)
    {
        return;
    }
    if ((g_task3_segment == previous_segment) &&
        (g_task3_segment_sent != 0U))
    {
        return;
    }
    if ((g_task3_segment_sent != 0U) &&
        (g_task3_segment == g_task3_last_sent_segment))
    {
        return;
    }

    Task3_SendCurrentSegment();
}

static uint8_t Task3_PointStopBit(Task3Segment segment)
{
    switch (segment)
    {
        case TASK3_SEG_BC:
            return 1U;
        case TASK3_SEG_CD:
            return 2U;
        case TASK3_SEG_DA:
            return 4U;
        case TASK3_SEG_AB:
        default:
            return 0U;
    }
}

static void Task3_StartPointStopIfNeeded(void)
{
#if TASK3_POINT_STOP_ENABLE
    uint8_t bit = Task3_PointStopBit(g_task3_segment);

    if ((bit == 0U) || ((g_task3_point_stop_mask & bit) != 0U))
    {
        return;
    }

    g_task3_point_stop_mask |= bit;
    g_task3_point_stop_active = 1U;
    g_task3_point_stop_segment = g_task3_segment;
    g_task3_point_stop_until_ms = board_millis() + TASK3_POINT_STOP_MS;
    SpeedPI_Reset();
    Motor_Brake();
#else
    (void)g_task3_segment;
#endif
}

static uint8_t Task3_RunPointStop(uint32_t now_ms)
{
    if (g_task3_point_stop_active == 0U)
    {
        return 0U;
    }

    SpeedPI_Reset();
    Motor_Brake();
    if ((now_ms - g_task3_point_stop_until_ms) < 0x80000000UL)
    {
        g_task3_point_stop_active = 0U;
        SpeedPI_Reset();
        return 0U;
    }

    return 1U;
}
static void Task3_HandleSegmentTransition(Task3Segment previous_segment)
{
    float curve_entry_ramp = 0.0f;

    if (g_task3_segment == previous_segment)
    {
        return;
    }

    if ((((previous_segment == TASK3_SEG_AB) &&
          (g_task3_segment == TASK3_SEG_BC)) ||
         ((previous_segment == TASK3_SEG_CD) &&
          (g_task3_segment == TASK3_SEG_DA))) &&
        (g_task3_pre_curve != 0U))
    {
        curve_entry_ramp = Task3_GetPreCurveRampFactor();
    }

    Task3_RecordSegmentEncoderStart();
    g_task3_curve_entry_ramp = curve_entry_ramp;

    if (((previous_segment == TASK3_SEG_BC) &&
         (g_task3_segment == TASK3_SEG_CD)) ||
        ((previous_segment == TASK3_SEG_DA) &&
         (g_task3_segment == TASK3_SEG_AB)))
    {
        g_task3_curve_entry_ramp = 0.0f;
        Task3_StartStraightTransition();
    }
    else if (Task3_IsCurveSegment() != 0U)
    {
        g_task3_straight_transition = 0U;
        g_task3_straight_transition_ms = 0U;
    }
    Task3_StartPointStopIfNeeded();
}

static void Task3_UpdateSegmentByEncoder(void)
{
    Task3Segment previous_segment = g_task3_segment;
    int32_t lap_target_count;

    g_task3_lap_advance_count = Task3_GetLapAdvanceCount();
    lap_target_count = Task3_GetLapTargetCount();
    if (lap_target_count > 0)
    {
        g_task3_progress_deg = Task3_ClampFloat(
            ((float)g_task3_lap_advance_count * 360.0f) /
            (float)lap_target_count,
            0.0f,
            359.9f);
    }
    else
    {
        g_task3_progress_deg = 0.0f;
    }

    g_task3_progress_ready = 1U;
    g_task3_segment = Task3_GetSegmentByLapCount(g_task3_lap_advance_count);
    Task3_HandleSegmentTransition(previous_segment);
    Task3_SendSegmentIfChanged(previous_segment);
}

static void Task3_SendI32(int32_t value)
{
    char digits[11];
    uint32_t magnitude;
    uint8_t count = 0U;

    if (value < 0)
    {
        StarFlash_SendByte((uint8_t)'-');
        magnitude = (uint32_t)(-value);
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    do
    {
        digits[count++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    } while (magnitude != 0U);

    while (count != 0U)
    {
        StarFlash_SendByte((uint8_t)digits[--count]);
    }
}

static void Task3_SendComma(void)
{
    StarFlash_SendByte((uint8_t)',');
}

static int32_t Task3_FloatToX10(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)((value * 10.0f) + 0.5f);
    }
    return (int32_t)((value * 10.0f) - 0.5f);
}

static int32_t Task3_FloatToX100(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)((value * 100.0f) + 0.5f);
    }
    return (int32_t)((value * 100.0f) - 0.5f);
}

static void Task3_SendDebugField(int32_t value)
{
    Task3_SendComma();
    Task3_SendI32(value);
}

static uint8_t Task3_GetParamMode(void)
{
    if (g_task3_straight_transition != 0U)
    {
        return 3U;
    }
    if (Task3_IsCurveSegment() != 0U)
    {
        return 2U;
    }
    if (g_task3_pre_curve != 0U)
    {
        return 1U;
    }
    return 0U;
}

static uint8_t Task3_IsFinishWindowSegment(void)
{
    return ((g_task3_segment == TASK3_SEG_DA) ||
            (g_task3_segment == TASK3_SEG_AB)) ? 1U : 0U;
}

static void Task3_UpdateFinishWindow(void)
{
    int32_t window_start_count;

    if (g_task3_finish_window != 0U)
    {
        return;
    }

    window_start_count = Task3_GetLapTargetCount() -
                         Task3_DistanceCmToCount(TASK3_FINISH_WINDOW_BEFORE_CM);
    if ((g_task3_lap_advance_count >= window_start_count) &&
        (Task3_IsFinishWindowSegment() != 0U))
    {
        g_task3_finish_window = 1U;
    }
}

static int32_t Task3_GetFinishStopCount(void)
{
    return Task3_GetLapTargetCount() -
           Task3_DistanceCmToCount(TASK3_FINISH_STOP_OFFSET_CM);
}

static int32_t Task3_GetBPlusStopTargetCount(void)
{
    return Task3_GetStraightTargetCount() +
           Task3_DistanceCmToCount(TASK3_B_PLUS_STOP_AFTER_B_CM);
}

static int32_t Task3_GetRunStopTargetCount(void)
{
    if (g_task3_run_mode == TASK3_RUN_B_PLUS_5CM)
    {
        return Task3_GetBPlusStopTargetCount();
    }
    return Task3_GetFinishStopCount();
}
static uint8_t Task3_ShouldStopAtFinish(void)
{
    int32_t stop_count = Task3_GetRunStopTargetCount();

    if (g_task3_lap_advance_count >= stop_count)
    {
        if (g_task3_finish_confirm_count < TASK3_FINISH_CONFIRM_COUNT)
        {
            g_task3_finish_confirm_count++;
        }
    }
    else
    {
        g_task3_finish_confirm_count = 0U;
    }

    return (g_task3_finish_confirm_count >= TASK3_FINISH_CONFIRM_COUNT) ? 1U : 0U;
}

static void Task3_SendDebugLine(void)
{
#if TASK3_DEBUG_STREAM
    uint32_t now_ms = board_millis();
    uint32_t csv_start_ms;

    if (g_task3_debug_stream_enabled == 0U)
    {
        return;
    }
    if ((now_ms - g_task3_next_debug_ms) >= 0x80000000UL)
    {
        return;
    }
    g_task3_next_debug_ms = now_ms + TASK3_DEBUG_LOG_MS;

    csv_start_ms = board_millis();
    Task3_SendI32((int32_t)g_task3_segment);
    Task3_SendDebugField((int32_t)g_task3_debug_mode);
    Task3_SendDebugField((int32_t)Task3_GetParamMode());
    Task3_SendDebugField((int32_t)Task3_LinkedOperation_GetProgressX10());
    Task3_SendDebugField(g_task3_segment_advance_count);
    Task3_SendDebugField(g_task3_lap_advance_count);
    Task3_SendDebugField((int32_t)g_task3_finish_window);
    Task3_SendDebugField((int32_t)g_task3_finish_confirm_count);
    Task3_SendDebugField(Task3_FloatToX100(Task3_GetPreCurveRampFactor()));
    Task3_SendDebugField(Task3_FloatToX100(Task3_GetCurveRampFactor()));
    Task3_SendDebugField((int32_t)g_task3_gray.error);
    Task3_SendDebugField(Task3_FloatToX10(g_task3_gray.correction_rpm));
    Task3_SendDebugField(Task3_FloatToX10(g_task3_gray.left_target_rpm));
    Task3_SendDebugField(Task3_FloatToX10(g_task3_gray.right_target_rpm));
    Task3_SendDebugField(Task3_FloatToX10(SpeedPI_GetLeftRPM()));
    Task3_SendDebugField(Task3_FloatToX10(SpeedPI_GetRightRPM()));
    Task3_SendDebugField(Task3_FloatToX10(g_task3_gray_base_rpm));
    Task3_SendDebugField(Task3_FloatToX100(g_task3_gray_kp_rpm));
    Task3_SendDebugField(Task3_FloatToX10(g_task3_gray_correction_max));
    Task3_SendDebugField((int32_t)g_task3_gray.black_mask);
    Task3_SendDebugField((int32_t)(g_task3_gray.line_detected == 0U));
    Task3_SendDebugField((int32_t)g_task3_curve_lost_hold);
    Task3_SendDebugField((int32_t)SpeedPI_GetLeftPWM());
    Task3_SendDebugField((int32_t)SpeedPI_GetRightPWM());
    StarFlash_SendString("\r\n");
    ControlTimingDiag_RecordCsvDuration(board_millis() - csv_start_ms);
#endif
}

#if TASK3_STRAIGHT_DIAG_MODE
static void Task3_SendStraightDiagLine(uint32_t elapsed_ms)
{
    uint32_t now_ms = board_millis();

    if ((now_ms - g_task3_next_debug_ms) >= 0x80000000UL)
    {
        return;
    }
    g_task3_next_debug_ms = now_ms + TASK3_DEBUG_LOG_MS;

    Task3_SendI32((int32_t)elapsed_ms);
    Task3_SendDebugField(Task3_FloatToX10(TASK3_STRAIGHT_DIAG_TARGET_RPM));
    Task3_SendDebugField(Task3_FloatToX10(TASK3_STRAIGHT_DIAG_TARGET_RPM));
    Task3_SendDebugField(Task3_FloatToX10(SpeedPI_GetLeftRPM()));
    Task3_SendDebugField(Task3_FloatToX10(SpeedPI_GetRightRPM()));
    Task3_SendDebugField(Encoder_GetLeftCount());
    Task3_SendDebugField(Encoder_GetRightCount());
    Task3_SendDebugField(Encoder_GetLeftDeltaCount());
    Task3_SendDebugField(Encoder_GetRightDeltaCount());
    Task3_SendDebugField((int32_t)SpeedPI_GetLeftPWM());
    Task3_SendDebugField((int32_t)SpeedPI_GetRightPWM());
    StarFlash_SendString("\r\n");
}

static void Task3_StraightDiagStep(void)
{
    uint32_t elapsed_ms = board_millis() - g_task3_start_ms;

    if (elapsed_ms >= TASK3_STRAIGHT_DIAG_MS)
    {
        SpeedPI_Reset();
        Motor_Coast();
        Task3_SendStraightDiagLine(elapsed_ms);
        g_task3_running = 0U;
        return;
    }

    SpeedPI_Update(TASK3_STRAIGHT_DIAG_TARGET_RPM,
                   TASK3_STRAIGHT_DIAG_TARGET_RPM);
    Task3_SendStraightDiagLine(elapsed_ms);
}
#endif

static void Task3_StopForLost(void)
{
    g_task3_curve_lost_hold = 0U;
    SpeedPI_Update(0.0f, 0.0f);
    Motor_Brake();
}

static void Task3_RunCurveLostHold(void)
{
    float left_hold;
    float right_hold;

    left_hold = g_task3_last_valid_left_rpm * TASK3_CURVE_LOST_HOLD_SCALE;
    right_hold = g_task3_last_valid_right_rpm * TASK3_CURVE_LOST_HOLD_SCALE;
    SpeedPI_Update(left_hold, right_hold);
}

static void Task3_HandleLostLine(void)
{
    if ((Task3_IsLostHoldAllowed() == 0U) ||
        (g_task3_has_valid_line == 0U))
    {
        Task3_StopForLost();
        return;
    }

    if (g_task3_curve_lost_hold == 0U)
    {
        g_task3_curve_lost_hold = 1U;
        g_task3_curve_lost_hold_ms = 0U;
    }
    else if (g_task3_curve_lost_hold_ms < 0xFFFFFFFFU - TASK3_CONTROL_MS)
    {
        g_task3_curve_lost_hold_ms += TASK3_CONTROL_MS;
    }

    if (g_task3_curve_lost_hold_ms <= TASK3_CURVE_LOST_HOLD_MS)
    {
        Task3_RunCurveLostHold();
    }
    else
    {
        Task3_StopForLost();
    }
}

static void Task3_ApplyGrayParams(void)
{
    uint8_t deadband = TASK3_NORMAL_ERROR_DEADBAND;
    float base_rpm = TASK3_STRAIGHT_BASE_RPM;
    float kp_rpm = TASK3_STRAIGHT_KP_RPM;
    float correction_max = TASK3_STRAIGHT_CORRECTION_MAX;
    float rise_step = TASK3_STRAIGHT_RISE_STEP;
    float fall_step = TASK3_STRAIGHT_FALL_STEP;

    if (Task3_IsCurveSegment() != 0U)
    {
        float ramp = Task3_GetCurveRampFactor();

        base_rpm = Task3_LerpFloat(TASK3_STRAIGHT_BASE_RPM,
                                   TASK3_CURVE_BASE_RPM,
                                   ramp);
        kp_rpm = Task3_LerpFloat(TASK3_STRAIGHT_KP_RPM,
                                 TASK3_CURVE_KP_RPM,
                                 ramp);
        correction_max = Task3_LerpFloat(TASK3_STRAIGHT_CORRECTION_MAX,
                                         TASK3_CURVE_CORRECTION_MAX,
                                         ramp);
        rise_step = Task3_LerpFloat(TASK3_STRAIGHT_RISE_STEP,
                                    TASK3_CURVE_RISE_STEP,
                                    ramp);
        fall_step = Task3_LerpFloat(TASK3_STRAIGHT_FALL_STEP,
                                    TASK3_CURVE_FALL_STEP,
                                    ramp);
    }
    else if (g_task3_pre_curve != 0U)
    {
        float ramp = Task3_GetPreCurveRampFactor();

        base_rpm = Task3_LerpFloat(TASK3_STRAIGHT_BASE_RPM,
                                   TASK3_PRE_CURVE_BASE_RPM,
                                   ramp);
        kp_rpm = Task3_LerpFloat(TASK3_STRAIGHT_KP_RPM,
                                 TASK3_PRE_CURVE_KP_RPM,
                                 ramp);
        correction_max = Task3_LerpFloat(TASK3_STRAIGHT_CORRECTION_MAX,
                                         TASK3_PRE_CURVE_CORRECTION_MAX,
                                         ramp);
        rise_step = Task3_LerpFloat(TASK3_STRAIGHT_RISE_STEP,
                                    TASK3_PRE_CURVE_RISE_STEP,
                                    ramp);
        fall_step = Task3_LerpFloat(TASK3_STRAIGHT_FALL_STEP,
                                    TASK3_PRE_CURVE_FALL_STEP,
                                    ramp);
    }

    Task3_SetGrayParamsSmooth(deadband,
                              base_rpm,
                              kp_rpm,
                              correction_max,
                              rise_step,
                              fall_step);
}

static void Task3_ControlStep(void)
{
    if (g_task3_running == 0U)
    {
        return;
    }

#if TASK3_STRAIGHT_DIAG_MODE
    Task3_StraightDiagStep();
    return;
#endif

    g_task3_debug_mode = Task3_GetParamMode();
    Task3_UpdateSegmentByEncoder();
    if (Task3_RunPointStop(board_millis()) != 0U)
    {
        g_task3_debug_mode = 8U;
        Task3_SendDebugLine();
        return;
    }
    if (Task3_ShouldStopAtBcExitCompStart() != 0U)
    {
        g_task3_debug_mode = 7U;
        SpeedPI_Reset();
        Motor_Brake();
        Task3_SendDebugLine();
        g_task3_running = 0U;
        return;
    }
    if (g_task3_lap_done != 0U)
    {
        g_task3_debug_mode = 6U;
        SpeedPI_Reset();
        Motor_Brake();
        Task3_SendDebugLine();
        g_task3_running = 0U;
        return;
    }
    Task3_UpdatePreCurveByEncoder();
    Task3_ApplyGrayParams();
    GrayTrack_Update();
    GrayTrack_GetOutput(&g_task3_gray);

    if (Gray_IsI2COk() == 0U)
    {
        g_task3_debug_mode = 5U;
        SpeedPI_Reset();
        Motor_Brake();
    }
    else if (Task3_ShouldStopAtFinish() != 0U)
    {
        g_task3_lap_done = 1U;
        g_task3_debug_mode = 6U;
        SpeedPI_Reset();
        Motor_Brake();
        Task3_SendDebugLine();
        g_task3_running = 0U;
        return;
    }
    else if (g_task3_gray.line_detected == 0U)
    {
        g_task3_debug_mode = 4U;
        Task3_HandleLostLine();
    }
    else
    {
        Task3_ApplyBcExitCompensation();
        g_task3_has_valid_line = 1U;
        g_task3_curve_lost_hold = 0U;
        g_task3_curve_lost_hold_ms = 0U;
        g_task3_last_valid_left_rpm = g_task3_gray.left_target_rpm;
        g_task3_last_valid_right_rpm = g_task3_gray.right_target_rpm;
        g_task3_debug_mode = Task3_GetParamMode();
        SpeedPI_Update(g_task3_gray.left_target_rpm,
                       g_task3_gray.right_target_rpm);
    }
    Task3_SendDebugLine();
    Task3_UpdateStraightTransitionTimer();
}

void Task3_LinkedOperation_StartMode(uint32_t now_ms, Task3_RunMode mode)
{
    Gray_Init();
    GrayTrack_Init();
    SpeedPI_Reset();

    g_task3_running = 1U;
    g_task3_next_control_ms = now_ms;
    g_task3_start_ms = now_ms;
    g_task3_run_mode = mode;
#if TASK3_STRAIGHT_DIAG_MODE
    Encoder_ClearCount();
#endif
    g_task3_has_valid_line = 0U;
    g_task3_last_valid_left_rpm = 0.0f;
    g_task3_last_valid_right_rpm = 0.0f;
    g_task3_curve_lost_hold = 0U;
    g_task3_curve_lost_hold_ms = 0U;
    g_task3_straight_transition = 0U;
    g_task3_straight_transition_ms = 0U;
    g_task3_progress_deg = 0.0f;
    g_task3_progress_ready = 0U;
    g_task3_segment = TASK3_SEG_AB;
    g_task3_last_sent_segment = TASK3_SEG_AB;
    g_task3_segment_sent = 0U;
    Task3_RecordLapEncoderStart();
    g_task3_curve_entry_ramp = 0.0f;
    g_task3_gray_params_ready = 0U;
    g_task3_next_debug_ms = now_ms;
    g_task3_debug_mode = 0U;
    g_task3_lap_done = 0U;
    g_task3_point_stop_active = 0U;
    g_task3_point_stop_until_ms = 0U;
    g_task3_point_stop_segment = TASK3_SEG_AB;
    g_task3_point_stop_mask = 0U;
    Task3_RecordSegmentEncoderStart();
    Task3_UpdateSegmentByEncoder();
}

void Task3_LinkedOperation_Start(uint32_t now_ms)
{
    Task3_LinkedOperation_StartMode(now_ms, TASK3_RUN_ONE_LAP);
}

void Task3_LinkedOperation_SetDebugEnabled(uint8_t enabled)
{
    g_task3_debug_stream_enabled = (enabled != 0U) ? 1U : 0U;
}

void Task3_LinkedOperation_Stop(void)
{
    g_task3_running = 0U;
    SpeedPI_Reset();
    Motor_Coast();
}

void Task3_LinkedOperation_Update(uint32_t now_ms)
{
    uint8_t steps = 0U;

    if (g_task3_running == 0U)
    {
        return;
    }

    while (((now_ms - g_task3_next_control_ms) < 0x80000000UL) &&
           (steps < 3U))
    {
        uint32_t control_start_ms = board_millis();

        ControlTimingDiag_ControlBegin(control_start_ms);
        Task3_ControlStep();
        ControlTimingDiag_ControlEnd(board_millis());
        g_task3_next_control_ms += TASK3_CONTROL_MS;
        steps++;
    }
    if (steps > 1U)
    {
        ControlTimingDiag_RecordCatchup((uint32_t)steps - 1U);
    }
    if ((now_ms - g_task3_next_control_ms) < 0x80000000UL)
    {
        ControlTimingDiag_RecordBacklogDrop();
        g_task3_next_control_ms = now_ms + TASK3_CONTROL_MS;
    }
}

uint8_t Task3_LinkedOperation_IsRunning(void)
{
    return g_task3_running;
}

const char *Task3_LinkedOperation_GetSegmentText(void)
{
    if (g_task3_progress_ready == 0U)
    {
        return "SEG: WAIT ENC";
    }
    if (g_task3_point_stop_active != 0U)
    {
        switch (g_task3_point_stop_segment)
        {
            case TASK3_SEG_BC:
                return "STOP: B";
            case TASK3_SEG_CD:
                return "STOP: C";
            case TASK3_SEG_DA:
                return "STOP: D";
            case TASK3_SEG_AB:
            default:
                return "STOP";
        }
    }

    if (g_task3_pre_curve != 0U)
    {
        if (g_task3_segment == TASK3_SEG_AB)
        {
            return "SEG: AB PRE";
        }
        if (g_task3_segment == TASK3_SEG_CD)
        {
            return "SEG: CD PRE";
        }
    }

    switch (g_task3_segment)
    {
        case TASK3_SEG_BC:
            return "SEG: BC";
        case TASK3_SEG_CD:
            return "SEG: CD";
        case TASK3_SEG_DA:
            return "SEG: DA";
        case TASK3_SEG_AB:
        default:
            return "SEG: AB";
    }
}

int16_t Task3_LinkedOperation_GetProgressX10(void)
{
    if (g_task3_progress_ready == 0U)
    {
        return -1;
    }
    return (int16_t)((g_task3_progress_deg * 10.0f) + 0.5f);
}

int32_t Task3_LinkedOperation_GetOdometerCount(void)
{
    return g_task3_lap_advance_count;
}
void Task3_LinkedOperation_Run(void)
{
    Task3_LinkedOperation_Start(board_millis());

    while (1)
    {
        Task3_LinkedOperation_Update(board_millis());
        delay_ms(5U);
    }
}

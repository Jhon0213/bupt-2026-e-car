#include <stdint.h>

#include "Application/RouteNavigator.h"

#include "Hardware/Bluetooth.h"
#include "Hardware/CONTROL/GrayTrack.h"
#include "Hardware/CONTROL/HeadingControl.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/IMU.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define CONTROL_MS                    ROUTE_NAVIGATOR_CONTROL_MS
#define LOG_MS                       100U
#define ROUTE_TIMEOUT_MS           20000U
#define IMU_READY_MS                1000U

#define AB_TARGET_DEG                  0.0f
#define CD_TARGET_DEG                180.0f
#define BC_SWITCH_PROGRESS_DEG       140.0f
#define C_ADVANCE_RPM                 80.0f
#define C_ADVANCE_COUNT            10470L
#define C_ADVANCE_TIMEOUT_MS        1800U
#define C_ALIGN_DONE_DEG             178.0f
#define C_ALIGN_TIMEOUT_MS          1300U
#define C_ALIGN_TOLERANCE_DEG          1.0f
#define C_ALIGN_CONFIRM_FRAMES           3U
#define C_ALIGN_STRICT_TIMEOUT_MS    2200U
#define C_BRAKE_MS                   120U
#define C_ALIGN_BASE_RPM              30.0f
#define DA_SWITCH_PROGRESS_DEG       320.0f
#define A_ADVANCE_RPM                 80.0f
#define A_ADVANCE_COUNT            10470L
#define A_ADVANCE_TIMEOUT_MS        1800U
#define A_ALIGN_TARGET_DEG           360.0f
#define A_ALIGN_DONE_DEG             358.0f
#define A_ALIGN_TIMEOUT_MS          1700U
#define A_ALIGN_TOLERANCE_DEG          0.8f
#define A_ALIGN_CONFIRM_FRAMES           5U
#define A_CONTINUE_ALIGN_TIMEOUT_MS  2400U
#define A_BRAKE_MS                   120U
#define A_ALIGN_BASE_RPM              30.0f
#define HEADING_BASE_RPM              80.0f
#define HEADING_KP                    2.25f
#define HEADING_KI                     0.0f
#define HEADING_KD                     1.0f
#define ADVANCE_HEADING_KP             1.6f
#define ADVANCE_HEADING_KI             0.0f
#define ADVANCE_HEADING_KD             0.6f
#define TARGET_SLEW_PER_YAW_FRAME      4.0f
#define TARGET_SLEW_RPM_PER_CONTROL    3.0f
#define HEADING_CORRECTION_LIMIT_RPM  60.0f
#define HEADING_TARGET_MIN_RPM         0.0f
#define HEADING_TARGET_MAX_RPM       180.0f
#define ALIGN_CORRECTION_LIMIT_RPM    30.0f
#define ALIGN_TARGET_MIN_RPM          10.0f

#define LINE_DETECT_DELAY_MS          500U
#define LINE_DETECT_CONFIRM             3U
#define CAPTURE_BASE_RPM              60.0f
#define CAPTURE_TARGET_MIN_RPM        25.0f
#define CAPTURE_TARGET_MAX_RPM        90.0f
#define CAPTURE_CENTER_ERROR            1
#define CAPTURE_CENTER_CONFIRM           5U
#define CAPTURE_TIMEOUT_MS           1500U
#define CAPTURE_EDGE_ERROR               3
#define CAPTURE_EDGE_BASE_RPM         45.0f
#define CAPTURE_EDGE_GAIN              1.5f
#define CAPTURE_EDGE_TARGET_MIN_RPM     5.0f

#define LOG_MAX                       190U
#define LOG_DETAIL_MS                  50U
#define LOG_EVENT_RESERVE               8U

enum
{
    STATE_HEADING_AB = 1,
    STATE_CAPTURE_B = 2,
    STATE_GRAY_BC = 3,
    STATE_ADVANCE_C = 4,
    STATE_ALIGN_C = 5,
    STATE_HEADING_CD = 6,
    STATE_CAPTURE_D = 7,
    STATE_GRAY_DA = 8,
    STATE_ADVANCE_A = 9,
    STATE_BRAKE_C = 10,
    STATE_WAIT_IMU = 11,
    STATE_BRAKE_A = 13,
    STATE_ALIGN_A = 14
};

enum
{
    STOP_LAP_COMPLETE = 1,
    STOP_EARLY_LOST = 2,
    STOP_ROUTE_TIMEOUT = 3,
    STOP_NO_IMU = 4,
    STOP_CAPTURE_TIMEOUT = 5,
    STOP_ALIGN_TIMEOUT = 7
};

enum
{
    LOG_EVENT_NONE = 0,
    LOG_EVENT_C_ADVANCE_ENTER = 1,
    LOG_EVENT_C_ADVANCE_COUNT = 2,
    LOG_EVENT_C_ADVANCE_TIMEOUT = 3,
    LOG_EVENT_C_ALIGN_YAW = 4,
    LOG_EVENT_C_ALIGN_TIMEOUT = 5,
    LOG_EVENT_A_ADVANCE_ENTER = 6,
    LOG_EVENT_A_ADVANCE_COUNT = 7,
    LOG_EVENT_A_ADVANCE_TIMEOUT = 8,
    LOG_EVENT_C_BRAKE_DONE = 9,
    LOG_EVENT_A_BRAKE_DONE = 10,
    LOG_EVENT_A_ALIGN_YAW = 11,
    LOG_EVENT_A_ALIGN_TIMEOUT = 12,
    LOG_EVENT_LAP_START = 13,
    LOG_EVENT_B_REACHED = 14,
    LOG_EVENT_D_REACHED = 15,
    LOG_EVENT_LAP_END = 16
};

typedef struct
{
    uint32_t ms;
    int16_t state;
    int16_t event;
    int16_t raw;
    int16_t black;
    int16_t gray_error;
    int32_t left_count;
    int32_t right_count;
    int32_t advance_count;
    int16_t yaw100;
    int16_t accum10;
    int16_t heading_target100;
    int16_t heading_error100;
    int16_t correction100;
    int16_t left_target10;
    int16_t right_target10;
    int16_t left_rpm10;
    int16_t right_rpm10;
    int16_t left_pwm;
    int16_t right_pwm;
} LogItem;

static LogItem logs[LOG_MAX];
static uint16_t log_count;
static uint8_t log_session_active;
static uint8_t log_lap;
static uint8_t log_detailed;
static uint8_t log_window_active;
static uint32_t log_time_offset;
static IMU_Data imu;
static GrayTrack_Output gray;
static HeadingControl_Output heading;
static uint32_t ms;
static uint32_t wait_ms;
static uint32_t next_log_ms;
static uint32_t yaw_frames;
static uint32_t state_ms;
static float last_yaw;
static float accumulated_yaw;
static float heading_command;
static float advance_heading_target;
static float left_target;
static float right_target;
static float left_command;
static float right_command;
static uint32_t route_events;
static uint8_t state;
static uint8_t reason;
static uint8_t line_confirm;
static uint8_t center_confirm;
static uint8_t align_confirm;
static uint8_t continuation_required;
static uint8_t log_event;
static uint8_t running;
static uint8_t paused;
static uint8_t finished;

static int16_t I16(float value)
{
    if (value > 32767.0f) return 32767;
    if (value < -32768.0f) return -32768;
    return (int16_t)value;
}

static float Clamp(float value, float minimum, float maximum)
{
    if (value > maximum) return maximum;
    if (value < minimum) return minimum;
    return value;
}

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static int16_t AbsInt16(int16_t value)
{
    return (value < 0) ? (int16_t)(-value) : value;
}

static int32_t AbsInt32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static float YawDelta(float now, float old)
{
    float delta = now - old;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return delta;
}

static float SlewToTarget(float value, float target)
{
    if (value < target)
    {
        value += TARGET_SLEW_PER_YAW_FRAME;
        if (value > target) value = target;
    }
    else if (value > target)
    {
        value -= TARGET_SLEW_PER_YAW_FRAME;
        if (value < target) value = target;
    }
    return value;
}

static float SlewRPMToTarget(float value, float target)
{
    if (value < target)
    {
        value += TARGET_SLEW_RPM_PER_CONTROL;
        if (value > target) value = target;
    }
    else if (value > target)
    {
        value -= TARGET_SLEW_RPM_PER_CONTROL;
        if (value < target) value = target;
    }
    return value;
}

static void SendU32(uint32_t value)
{
    char digits[10];
    uint32_t count = 0U;

    do
    {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);

    while (count != 0U)
    {
        Bluetooth_SendByte((uint8_t)digits[--count]);
    }
}

static void SendI32(int32_t value)
{
    if (value < 0)
    {
        Bluetooth_SendByte((uint8_t)'-');
        SendU32((uint32_t)(-(value + 1)) + 1U);
    }
    else
    {
        SendU32((uint32_t)value);
    }
}

static void SendField(int32_t value)
{
    Bluetooth_SendByte((uint8_t)',');
    SendI32(value);
}

static void SendLine(const char *text)
{
    Bluetooth_SendString(text);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

static void SendParameters(void)
{
    Bluetooth_SendString("P");
    SendField(10);
    SendField(CONTROL_MS);
    SendField(LOG_MS);
    SendField(LOG_DETAIL_MS);
    SendField(14000);
    SendField(33);
    SendField(C_ADVANCE_COUNT);
    SendField(I16(C_ADVANCE_RPM * 10.0f));
    SendField(C_ADVANCE_TIMEOUT_MS);
    SendField(A_ADVANCE_COUNT);
    SendField(I16(A_ADVANCE_RPM * 10.0f));
    SendField(A_ADVANCE_TIMEOUT_MS);
    SendField(I16(HEADING_BASE_RPM * 10.0f));
    SendField(I16(HEADING_KP * 100.0f));
    SendField(I16(HEADING_KD * 100.0f));
    SendField(I16(HEADING_CORRECTION_LIMIT_RPM * 10.0f));
    SendField(I16(HEADING_TARGET_MIN_RPM * 10.0f));
    SendField(I16(C_ALIGN_BASE_RPM * 10.0f));
    SendField(I16(A_ALIGN_BASE_RPM * 10.0f));
    SendField(I16(ALIGN_CORRECTION_LIMIT_RPM * 10.0f));
    SendField(I16(ALIGN_TARGET_MIN_RPM * 10.0f));
    SendField(I16(TARGET_SLEW_RPM_PER_CONTROL * 10.0f));
    SendField(I16(GrayTrack_GetBaseRPM() * 10.0f));
    SendField(I16(GrayTrack_GetKpRPM() * 100.0f));
    SendField(I16(GrayTrack_GetKdRPM() * 100.0f));
    SendField(I16(GrayTrack_GetCorrectionMaxRPM() * 10.0f));
    SendField(I16(GrayTrack_GetTargetMinRPM() * 10.0f));
    SendField(I16(GrayTrack_GetTargetMaxRPM() * 10.0f));
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

static void SaveLog(uint32_t sample_ms, uint8_t sample_state,
                    uint8_t event, float yaw, float accum,
                    const GrayTrack_Output *gray,
                    const HeadingControl_Output *heading,
                    float sample_left_target, float sample_right_target)
{
    LogItem *item;
    int32_t left_count;
    int32_t right_count;

    if ((event == LOG_EVENT_NONE) &&
        (log_count >= (LOG_MAX - LOG_EVENT_RESERVE))) return;

    if (log_count >= LOG_MAX)
    {
        if (event == LOG_EVENT_NONE) return;
        item = &logs[LOG_MAX - 1U];
    }
    else
    {
        item = &logs[log_count++];
    }

    item->ms = log_time_offset + sample_ms;
    item->state = (int16_t)(((uint16_t)log_lap * 100U) + sample_state);
    item->event = event;
    item->raw = (int16_t)((uint16_t)gray->raw |
                          ((uint16_t)line_confirm << 8) |
                          ((uint16_t)center_confirm << 11));
    item->black = (int16_t)((uint16_t)gray->black_mask |
                            ((uint16_t)gray->line_detected << 8) |
                            ((uint16_t)gray->lost_confirmed << 9));
    item->gray_error = gray->error;
    left_count = Encoder_GetLeftCount();
    right_count = Encoder_GetRightCount();
    item->left_count = left_count;
    item->right_count = right_count;
    item->advance_count =
        (AbsInt32(left_count) + AbsInt32(right_count)) / 2;
    item->yaw100 = I16(yaw * 100.0f);
    item->accum10 = I16(accum * 10.0f);
    item->heading_target100 = I16(heading->target_yaw_deg * 100.0f);
    item->heading_error100 = I16(heading->error_deg * 100.0f);
    item->correction100 = I16(heading->correction_rpm * 100.0f);
    item->left_target10 = I16(sample_left_target * 10.0f);
    item->right_target10 = I16(sample_right_target * 10.0f);
    item->left_rpm10 = I16(SpeedPI_GetLeftRPM() * 10.0f);
    item->right_rpm10 = I16(SpeedPI_GetRightRPM() * 10.0f);
    item->left_pwm = (int16_t)SpeedPI_GetLeftPWM();
    item->right_pwm = (int16_t)SpeedPI_GetRightPWM();
}

static void SendLogs(uint8_t final_reason, uint8_t final_state,
                     float final_accum)
{
    uint16_t i;

    SendLine("S");
    SendLine("PF,version,control_ms,log_ms,detail_log_ms,pulse_per_cycle,wheel_radius_mm,c_count,c_rpm_x10,c_timeout_ms,a_count,a_rpm_x10,a_timeout_ms,heading_base_x10,heading_kp_x100,heading_kd_x100,heading_limit_x10,heading_min_x10,c_align_base_x10,a_align_base_x10,align_limit_x10,align_min_x10,target_slew_x10,gray_base_x10,gray_kp_x100,gray_kd_x100,gray_limit_x10,gray_min_x10,gray_max_x10");
    SendParameters();
    SendLine("M,event,0=NONE,1=C_ADV_ENTER,2=C_ADV_COUNT,3=C_ADV_TIMEOUT,4=C_ALIGN_YAW,5=C_ALIGN_TIMEOUT,6=A_ADV_ENTER,7=A_ADV_COUNT,8=A_ADV_TIMEOUT,9=C_BRAKE_DONE,10=A_BRAKE_DONE,11=A_ALIGN_YAW,12=A_ALIGN_TIMEOUT,13=LAP_START,14=B_REACHED,15=D_REACHED,16=LAP_END");
    SendLine("F,time_ms,lap,state,event,raw,black,line_detected,lost_confirmed,line_confirm,center_confirm,gray_error,left_count,right_count,advance_count,yaw_x100,accum_x10,heading_target_x100,heading_error_x100,correction_x100,left_target_x10,right_target_x10,left_rpm_x10,right_rpm_x10,left_pwm,right_pwm");

    for (i = 0U; i < log_count; i++)
    {
        LogItem *item = &logs[i];
        int16_t encoded_state = item->state;
        int16_t packed_raw = item->raw;
        int16_t packed_black = item->black;

        Bluetooth_SendString("D,");
        SendU32(item->ms);
        SendField(encoded_state / 100);
        SendField(encoded_state % 100);
        SendField(item->event);
        SendField(packed_raw & 0xFF);
        SendField(packed_black & 0xFF);
        SendField((packed_black >> 8) & 0x01);
        SendField((packed_black >> 9) & 0x01);
        SendField((packed_raw >> 8) & 0x07);
        SendField((packed_raw >> 11) & 0x07);
        SendField(item->gray_error);
        SendField(item->left_count);
        SendField(item->right_count);
        SendField(item->advance_count);
        SendField(item->yaw100);
        SendField(item->accum10);
        SendField(item->heading_target100);
        SendField(item->heading_error100);
        SendField(item->correction100);
        SendField(item->left_target10);
        SendField(item->right_target10);
        SendField(item->left_rpm10);
        SendField(item->right_rpm10);
        SendField(item->left_pwm);
        SendField(item->right_pwm);
        Bluetooth_SendByte(13U);
        Bluetooth_SendByte(10U);
    }

    Bluetooth_SendString("R,");
    SendU32(final_reason);
    SendField(log_lap);
    SendField(final_state);
    SendField((int32_t)(final_accum * 100.0f));
    SendField(log_count);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
    SendLine("E");
}

static uint8_t IsDetailedLogState(uint8_t sample_state)
{
    return ((sample_state == STATE_HEADING_AB) ||
            (sample_state == STATE_CAPTURE_B) ||
            (sample_state == STATE_ADVANCE_C) ||
            (sample_state == STATE_BRAKE_C) ||
            (sample_state == STATE_ALIGN_C) ||
            (sample_state == STATE_HEADING_CD) ||
            (sample_state == STATE_CAPTURE_D)) ? 1U : 0U;
}

static uint32_t DetailedLogPeriod(uint8_t sample_state)
{
    if ((sample_state == STATE_HEADING_AB) ||
        (sample_state == STATE_CAPTURE_B)) return LOG_MS;
    return LOG_DETAIL_MS;
}
static void StartHeading(float current_yaw, float final_target,
                         float base_rpm, float *command_target)
{
    *command_target = current_yaw;
    HeadingControl_SetTarget(*command_target, base_rpm);
    HeadingControl_Reset(current_yaw);
    (void)final_target;
}

static void UpdateHeading(float actual_yaw, float final_target,
                          float base_rpm, float *command_target)
{
    *command_target = SlewToTarget(*command_target, final_target);
    HeadingControl_SetTarget(*command_target, base_rpm);
    HeadingControl_Update(actual_yaw);
}

static void SetCaptureTargets(const GrayTrack_Output *gray,
                              float *left_target, float *right_target)
{
    float base_rpm = CAPTURE_BASE_RPM;
    float correction_rpm = gray->correction_rpm;
    float minimum_rpm = CAPTURE_TARGET_MIN_RPM;

    if (AbsInt16(gray->error) >= CAPTURE_EDGE_ERROR)
    {
        base_rpm = CAPTURE_EDGE_BASE_RPM;
        correction_rpm *= CAPTURE_EDGE_GAIN;
        minimum_rpm = CAPTURE_EDGE_TARGET_MIN_RPM;
    }

    *left_target =
        Clamp(base_rpm + correction_rpm,
              minimum_rpm,
              CAPTURE_TARGET_MAX_RPM);
    *right_target =
        Clamp(base_rpm - correction_rpm,
              minimum_rpm,
              CAPTURE_TARGET_MAX_RPM);
}
static void FinishRoute(uint8_t finish_reason)
{
    if (finished != 0U) return;

    reason = finish_reason;
    GrayTrack_GetOutput(&gray);
    HeadingControl_GetOutput(&heading);
    SaveLog(ms, state,
            (log_session_active != 0U) ? LOG_EVENT_LAP_END : LOG_EVENT_NONE,
            imu.yaw_deg, accumulated_yaw,
            &gray, &heading, left_command, right_command);

    SpeedPI_Reset();
    Motor_Brake();
    running = 0U;
    paused = 0U;
    finished = 1U;

    if (reason == STOP_LAP_COMPLETE)
    {
        route_events |= ROUTE_EVENT_REACHED_A | ROUTE_EVENT_LAP_COMPLETE;
    }
    else
    {
        route_events |= ROUTE_EVENT_ERROR;
    }
}

void RouteNavigator_SetContinuationRequired(uint8_t required)
{
    continuation_required = (required != 0U) ? 1U : 0U;
}
void RouteNavigator_LogSessionBegin(void)
{
    log_session_active = 1U;
    log_count = 0U;
    log_lap = 0U;
    log_detailed = 0U;
    log_window_active = 0U;
    log_time_offset = 0U;
}

void RouteNavigator_LogSessionSelectLap(uint8_t lap, uint8_t detailed)
{
    if (log_session_active == 0U) return;

    if ((log_lap != 0U) && (lap != log_lap))
        log_time_offset += ms;

    log_lap = (lap == 0U) ? 1U : lap;
    log_detailed = (detailed != 0U) ? 1U : 0U;
    log_window_active = 0U;
}
void RouteNavigator_Start(void)
{
    if (log_session_active == 0U)
    {
        log_count = 0U;
        log_lap = 1U;
        log_detailed = 0U;
        log_time_offset = 0U;
    }
    else if (log_lap == 0U)
    {
        log_lap = 1U;
    }

    log_window_active = 0U;
    ms = 0U;
    wait_ms = 0U;
    next_log_ms = LOG_MS;
    yaw_frames = 0U;
    state_ms = 0U;
    last_yaw = 0.0f;
    accumulated_yaw = 0.0f;
    heading_command = 0.0f;
    advance_heading_target = 0.0f;
    left_target = 0.0f;
    right_target = 0.0f;
    left_command = 0.0f;
    right_command = 0.0f;
    route_events = ROUTE_EVENT_NONE;
    state = STATE_WAIT_IMU;
    reason = 0U;
    line_confirm = 0U;
    center_confirm = 0U;
    align_confirm = 0U;
    log_event = (log_session_active != 0U) ?
        LOG_EVENT_LAP_START : LOG_EVENT_NONE;
    running = 1U;
    paused = 0U;
    finished = 0U;
    imu.yaw_deg = 0.0f;
    imu.yaw_frame_count = 0U;

    GrayTrack_Reset();
    SpeedPI_Reset();
    HeadingControl_Init();
    HeadingControl_SetOutputLimits(HEADING_CORRECTION_LIMIT_RPM,
                                   HEADING_TARGET_MIN_RPM,
                                   HEADING_TARGET_MAX_RPM);
    Encoder_ClearCount();
    Motor_Coast();
}

__attribute__((optnone)) void RouteNavigator_Update(void)
{
    uint8_t new_yaw = 0U;

    if ((running == 0U) || (paused != 0U) || (finished != 0U)) return;

    if (state == STATE_WAIT_IMU)
    {
        IMU_GetData(&imu);
        if (imu.yaw_frame_count != 0U)
        {
            last_yaw = imu.yaw_deg;
            yaw_frames = imu.yaw_frame_count;
            state = STATE_HEADING_AB;
            state_ms = 0U;
            Encoder_ClearCount();
            HeadingControl_SetGains(HEADING_KP, HEADING_KI, HEADING_KD);
            HeadingControl_SetTarget(AB_TARGET_DEG, HEADING_BASE_RPM);
            HeadingControl_Reset(0.0f);
        }
        else
        {
            wait_ms += CONTROL_MS;
            if (wait_ms >= IMU_READY_MS) FinishRoute(STOP_NO_IMU);
        }
        return;
    }
            ms += CONTROL_MS;
            state_ms += CONTROL_MS;
            IMU_GetData(&imu);

            if (imu.yaw_frame_count != yaw_frames)
            {
                accumulated_yaw += YawDelta(imu.yaw_deg, last_yaw);
                last_yaw = imu.yaw_deg;
                yaw_frames = imu.yaw_frame_count;
                new_yaw = 1U;
            }

            GrayTrack_Update();
            GrayTrack_GetOutput(&gray);

            if (state == STATE_HEADING_AB)
            {
                if (new_yaw != 0U)
                {
                    HeadingControl_SetTarget(AB_TARGET_DEG,
                                             HEADING_BASE_RPM);
                    HeadingControl_Update(accumulated_yaw);
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if (state_ms >= LINE_DETECT_DELAY_MS)
                {
                    if (gray.line_detected != 0U)
                    {
                        if (line_confirm < LINE_DETECT_CONFIRM)
                            line_confirm++;
                        if (line_confirm >= LINE_DETECT_CONFIRM)
                        {
                            state = STATE_CAPTURE_B;
                            state_ms = 0U;
                            center_confirm = 0U;
                            GrayTrack_Reset();
                        }
                    }
                    else
                    {
                        line_confirm = 0U;
                    }
                }
            }
            else if (state == STATE_CAPTURE_B)
            {
                if (gray.line_detected != 0U)
                {
                    SetCaptureTargets(&gray, &left_target, &right_target);

                    if (AbsInt16(gray.error) <= CAPTURE_CENTER_ERROR)
                    {
                        if (center_confirm < CAPTURE_CENTER_CONFIRM)
                            center_confirm++;
                        if (center_confirm >= CAPTURE_CENTER_CONFIRM)
                        {
                            state = STATE_GRAY_BC;
                            route_events |= ROUTE_EVENT_REACHED_B;
                            log_event = LOG_EVENT_B_REACHED;
                            state_ms = 0U;
                            line_confirm = 0U;
                        }
                    }
                    else
                    {
                        center_confirm = 0U;
                    }
                }

                if (state_ms >= CAPTURE_TIMEOUT_MS)
                {
                    reason = STOP_CAPTURE_TIMEOUT;
                }
            }
            else if (state == STATE_GRAY_BC)
            {
                if (gray.line_detected != 0U)
                {
                    left_target = gray.left_target_rpm;
                    right_target = gray.right_target_rpm;
                }
                else
                {
                    /* Cancel stale gray differential while loss is confirmed. */
                    left_target = C_ADVANCE_RPM;
                    right_target = C_ADVANCE_RPM;
                }

                if ((state_ms >= LINE_DETECT_DELAY_MS) &&
                    (gray.lost_confirmed != 0U))
                {
                    if (accumulated_yaw >= BC_SWITCH_PROGRESS_DEG)
                    {
                        advance_heading_target = accumulated_yaw;
                        state = STATE_ADVANCE_C;
                        state_ms = 0U;
                        line_confirm = 0U;
                        Encoder_ClearCount();
                        SpeedPI_BalanceForStraight(C_ADVANCE_RPM);
                        HeadingControl_SetGains(ADVANCE_HEADING_KP,
                                                ADVANCE_HEADING_KI,
                                                ADVANCE_HEADING_KD);
                        HeadingControl_SetTarget(advance_heading_target,
                                                 C_ADVANCE_RPM);
                        HeadingControl_Reset(accumulated_yaw);
                        HeadingControl_GetOutput(&heading);
                        left_target = heading.left_target_rpm;
                        right_target = heading.right_target_rpm;
                        log_event = LOG_EVENT_C_ADVANCE_ENTER;
                    }
                    else
                    {
                        reason = STOP_EARLY_LOST;
                    }
                }
            }
            else if (state == STATE_ADVANCE_C)
            {
                int32_t left_count = AbsInt32(Encoder_GetLeftCount());
                int32_t right_count = AbsInt32(Encoder_GetRightCount());
                int32_t advance_count = (left_count + right_count) / 2;

                if (new_yaw != 0U)
                {
                    HeadingControl_SetTarget(advance_heading_target,
                                             C_ADVANCE_RPM);
                    HeadingControl_Update(accumulated_yaw);
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if (advance_count >= C_ADVANCE_COUNT)
                {
                    log_event = LOG_EVENT_C_ADVANCE_COUNT;
                    state = STATE_BRAKE_C;
                    state_ms = 0U;
                    line_confirm = 0U;
                    left_target = 0.0f;
                    right_target = 0.0f;
                    SpeedPI_Reset();
                    Motor_Brake();
                }
                else if (state_ms >= C_ADVANCE_TIMEOUT_MS)
                {
                    log_event = LOG_EVENT_C_ADVANCE_TIMEOUT;
                    state = STATE_BRAKE_C;
                    state_ms = 0U;
                    line_confirm = 0U;
                    left_target = 0.0f;
                    right_target = 0.0f;
                    SpeedPI_Reset();
                    Motor_Brake();
                }
            }
            else if (state == STATE_BRAKE_C)
            {
                left_target = 0.0f;
                right_target = 0.0f;
                Motor_Brake();

                if (state_ms >= C_BRAKE_MS)
                {
                    log_event = LOG_EVENT_C_BRAKE_DONE;
                    state = STATE_ALIGN_C;
                    route_events |= ROUTE_EVENT_REACHED_C;
                    state_ms = 0U;
                    align_confirm = 0U;
                    heading_command = CD_TARGET_DEG;
                    HeadingControl_SetGains(HEADING_KP, HEADING_KI,
                                            HEADING_KD);
                    HeadingControl_SetOutputLimits(ALIGN_CORRECTION_LIMIT_RPM,
                                                   ALIGN_TARGET_MIN_RPM,
                                                   HEADING_TARGET_MAX_RPM);
                    HeadingControl_SetTarget(CD_TARGET_DEG,
                                             C_ALIGN_BASE_RPM);
                    HeadingControl_Reset(accumulated_yaw);
                    HeadingControl_Update(accumulated_yaw);
                    HeadingControl_GetOutput(&heading);
                    left_target = heading.left_target_rpm;
                    right_target = heading.right_target_rpm;
                }
            }
            else if (state == STATE_ALIGN_C)
            {
                if (new_yaw != 0U)
                {
                    HeadingControl_SetTarget(CD_TARGET_DEG,
                                             C_ALIGN_BASE_RPM);
                    HeadingControl_Update(accumulated_yaw);

                    if (AbsFloat(CD_TARGET_DEG - accumulated_yaw) <=
                        C_ALIGN_TOLERANCE_DEG)
                    {
                        if (align_confirm < C_ALIGN_CONFIRM_FRAMES)
                            align_confirm++;
                    }
                    else
                    {
                        align_confirm = 0U;
                    }
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if (align_confirm >= C_ALIGN_CONFIRM_FRAMES)
                {
                    log_event = LOG_EVENT_C_ALIGN_YAW;
                    state = STATE_HEADING_CD;
                    state_ms = 0U;
                    line_confirm = 0U;
                    HeadingControl_SetOutputLimits(HEADING_CORRECTION_LIMIT_RPM,
                                                   HEADING_TARGET_MIN_RPM,
                                                   HEADING_TARGET_MAX_RPM);
                    HeadingControl_SetTarget(heading_command,
                                             HEADING_BASE_RPM);
                    HeadingControl_GetOutput(&heading);
                    left_target = heading.left_target_rpm;
                    right_target = heading.right_target_rpm;
                }
                else if (state_ms >= C_ALIGN_STRICT_TIMEOUT_MS)
                {
                    log_event = LOG_EVENT_C_ALIGN_TIMEOUT;
                    state = STATE_HEADING_CD;
                    state_ms = 0U;
                    line_confirm = 0U;
                    HeadingControl_SetOutputLimits(HEADING_CORRECTION_LIMIT_RPM,
                                                   HEADING_TARGET_MIN_RPM,
                                                   HEADING_TARGET_MAX_RPM);
                    HeadingControl_SetTarget(heading_command,
                                             HEADING_BASE_RPM);
                    HeadingControl_GetOutput(&heading);
                    left_target = heading.left_target_rpm;
                    right_target = heading.right_target_rpm;
                }
            }
            else if (state == STATE_HEADING_CD)
            {
                if (new_yaw != 0U)
                {
                    UpdateHeading(accumulated_yaw, CD_TARGET_DEG,
                                  HEADING_BASE_RPM, &heading_command);
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if ((state_ms >= LINE_DETECT_DELAY_MS) &&
                    (accumulated_yaw >= 170.0f))
                {
                    if (gray.line_detected != 0U)
                    {
                        if (line_confirm < LINE_DETECT_CONFIRM)
                            line_confirm++;
                        if (line_confirm >= LINE_DETECT_CONFIRM)
                        {
                            state = STATE_CAPTURE_D;
                            state_ms = 0U;
                            center_confirm = 0U;
                            GrayTrack_Reset();
                        }
                    }
                    else
                    {
                        line_confirm = 0U;
                    }
                }
            }
            else if (state == STATE_CAPTURE_D)
            {
                if (gray.line_detected != 0U)
                {
                    SetCaptureTargets(&gray, &left_target, &right_target);

                    if (AbsInt16(gray.error) <= CAPTURE_CENTER_ERROR)
                    {
                        if (center_confirm < CAPTURE_CENTER_CONFIRM)
                            center_confirm++;
                        if (center_confirm >= CAPTURE_CENTER_CONFIRM)
                        {
                            state = STATE_GRAY_DA;
                            route_events |= ROUTE_EVENT_REACHED_D;
                            log_event = LOG_EVENT_D_REACHED;
                            state_ms = 0U;
                            line_confirm = 0U;
                        }
                    }
                    else
                    {
                        center_confirm = 0U;
                    }
                }

                if (state_ms >= CAPTURE_TIMEOUT_MS)
                {
                    reason = STOP_CAPTURE_TIMEOUT;
                }
            }
            else if (state == STATE_GRAY_DA)
            {
                if (gray.line_detected != 0U)
                {
                    left_target = gray.left_target_rpm;
                    right_target = gray.right_target_rpm;
                }
                else
                {
                    /* Cancel stale gray differential while loss is confirmed. */
                    left_target = A_ADVANCE_RPM;
                    right_target = A_ADVANCE_RPM;
                }

                if ((state_ms >= LINE_DETECT_DELAY_MS) &&
                    (gray.lost_confirmed != 0U))
                {
                    if (accumulated_yaw >= DA_SWITCH_PROGRESS_DEG)
                    {
                        advance_heading_target = accumulated_yaw;
                        state = STATE_ADVANCE_A;
                        state_ms = 0U;
                        Encoder_ClearCount();
                        SpeedPI_BalanceForStraight(A_ADVANCE_RPM);
                        HeadingControl_SetGains(ADVANCE_HEADING_KP,
                                                ADVANCE_HEADING_KI,
                                                ADVANCE_HEADING_KD);
                        HeadingControl_SetTarget(advance_heading_target,
                                                 A_ADVANCE_RPM);
                        HeadingControl_Reset(accumulated_yaw);
                        HeadingControl_GetOutput(&heading);
                        left_target = heading.left_target_rpm;
                        right_target = heading.right_target_rpm;
                        log_event = LOG_EVENT_A_ADVANCE_ENTER;
                    }
                    else
                    {
                        reason = STOP_EARLY_LOST;
                    }
                }
            }
            else if (state == STATE_ADVANCE_A)
            {
                int32_t left_count = AbsInt32(Encoder_GetLeftCount());
                int32_t right_count = AbsInt32(Encoder_GetRightCount());
                int32_t advance_count = (left_count + right_count) / 2;

                if (new_yaw != 0U)
                {
                    HeadingControl_SetTarget(advance_heading_target,
                                             A_ADVANCE_RPM);
                    HeadingControl_Update(accumulated_yaw);
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if (advance_count >= A_ADVANCE_COUNT)
                {
                    log_event = LOG_EVENT_A_ADVANCE_COUNT;
                    state = STATE_BRAKE_A;
                    state_ms = 0U;
                    left_target = 0.0f;
                    right_target = 0.0f;
                    SpeedPI_Reset();
                    Motor_Brake();
                }
                else if (state_ms >= A_ADVANCE_TIMEOUT_MS)
                {
                    log_event = LOG_EVENT_A_ADVANCE_TIMEOUT;
                    state = STATE_BRAKE_A;
                    state_ms = 0U;
                    left_target = 0.0f;
                    right_target = 0.0f;
                    SpeedPI_Reset();
                    Motor_Brake();
                }
            }
            else if (state == STATE_BRAKE_A)
            {
                left_target = 0.0f;
                right_target = 0.0f;
                Motor_Brake();

                if (state_ms >= A_BRAKE_MS)
                {
                    log_event = LOG_EVENT_A_BRAKE_DONE;
                    state = STATE_ALIGN_A;
                    state_ms = 0U;
                    align_confirm = 0U;
                    HeadingControl_SetGains(HEADING_KP, HEADING_KI,
                                            HEADING_KD);
                    HeadingControl_SetOutputLimits(ALIGN_CORRECTION_LIMIT_RPM,
                                                   ALIGN_TARGET_MIN_RPM,
                                                   HEADING_TARGET_MAX_RPM);
                    HeadingControl_SetTarget(A_ALIGN_TARGET_DEG,
                                             A_ALIGN_BASE_RPM);
                    HeadingControl_Reset(accumulated_yaw);
                    HeadingControl_Update(accumulated_yaw);
                    HeadingControl_GetOutput(&heading);
                    left_target = heading.left_target_rpm;
                    right_target = heading.right_target_rpm;
                }
            }
            else if (state == STATE_ALIGN_A)
            {
                if (new_yaw != 0U)
                {
                    HeadingControl_SetTarget(A_ALIGN_TARGET_DEG,
                                             A_ALIGN_BASE_RPM);
                    HeadingControl_Update(accumulated_yaw);

                    if (continuation_required != 0U)
                    {
                        if (AbsFloat(A_ALIGN_TARGET_DEG - accumulated_yaw) <=
                            A_ALIGN_TOLERANCE_DEG)
                        {
                            if (align_confirm < A_ALIGN_CONFIRM_FRAMES)
                                align_confirm++;
                        }
                        else
                        {
                            align_confirm = 0U;
                        }
                    }
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if (continuation_required != 0U)
                {
                    if (align_confirm >= A_ALIGN_CONFIRM_FRAMES)
                    {
                        log_event = LOG_EVENT_A_ALIGN_YAW;
                        reason = STOP_LAP_COMPLETE;
                    }
                    else if (state_ms >= A_CONTINUE_ALIGN_TIMEOUT_MS)
                    {
                        log_event = LOG_EVENT_A_ALIGN_TIMEOUT;
                        reason = STOP_ALIGN_TIMEOUT;
                    }
                }
                else if (accumulated_yaw >= A_ALIGN_DONE_DEG)
                {
                    log_event = LOG_EVENT_A_ALIGN_YAW;
                    reason = STOP_LAP_COMPLETE;
                }
                else if (state_ms >= A_ALIGN_TIMEOUT_MS)
                {
                    log_event = LOG_EVENT_A_ALIGN_TIMEOUT;
                    reason = STOP_LAP_COMPLETE;
                }
            }
            HeadingControl_GetOutput(&heading);

            if ((state == STATE_BRAKE_C) || (state == STATE_BRAKE_A))
            {
                left_command = 0.0f;
                right_command = 0.0f;
            }
            else if (reason == 0U)
            {
                left_command = SlewRPMToTarget(left_command, left_target);
                right_command = SlewRPMToTarget(right_command, right_target);
                SpeedPI_Update(left_command, right_command);
            }

            if ((log_session_active != 0U) && (log_detailed != 0U))
            {
                if (IsDetailedLogState(state) != 0U)
                {
                    if (log_window_active == 0U)
                    {
                        log_window_active = 1U;
                        next_log_ms = ms;
                    }
                }
                else
                {
                    log_window_active = 0U;
                }
            }

            if (log_event != LOG_EVENT_NONE)
            {
                SaveLog(ms, state, log_event,
                        imu.yaw_deg, accumulated_yaw,
                        &gray, &heading, left_command, right_command);
                log_event = LOG_EVENT_NONE;

                if ((log_session_active != 0U) &&
                    (log_detailed != 0U) &&
                    (log_window_active != 0U))
                {
                    next_log_ms = ms + DetailedLogPeriod(state);
                }
                else if ((log_session_active == 0U) &&
                         (ms >= next_log_ms))
                {
                    next_log_ms += LOG_MS;
                }
            }
            else if ((log_session_active != 0U) &&
                     (log_detailed != 0U) &&
                     (log_window_active != 0U) &&
                     (ms >= next_log_ms))
            {
                SaveLog(ms, state, LOG_EVENT_NONE,
                        imu.yaw_deg, accumulated_yaw,
                        &gray, &heading, left_command, right_command);
                next_log_ms = ms + DetailedLogPeriod(state);
            }
            else if ((log_session_active == 0U) &&
                     (ms >= next_log_ms))
            {
                SaveLog(ms, state, LOG_EVENT_NONE,
                        imu.yaw_deg, accumulated_yaw,
                        &gray, &heading, left_command, right_command);
                next_log_ms += LOG_MS;
            }
    if ((reason == 0U) && (ms >= ROUTE_TIMEOUT_MS))
    {
        reason = STOP_ROUTE_TIMEOUT;
    }
    if (reason != 0U)
    {
        FinishRoute(reason);
    }
}

void RouteNavigator_Pause(void)
{
    if ((running == 0U) || (paused != 0U)) return;
    paused = 1U;
    SpeedPI_Reset();
    Motor_Brake();
}

void RouteNavigator_Resume(void)
{
    if ((running == 0U) || (paused == 0U)) return;
    paused = 0U;
}

void RouteNavigator_Abort(void)
{
    if (running != 0U) FinishRoute(ROUTE_RESULT_ABORTED);
}

uint8_t RouteNavigator_IsRunning(void)
{
    return running;
}

uint8_t RouteNavigator_IsPaused(void)
{
    return paused;
}

uint8_t RouteNavigator_IsFinished(void)
{
    return finished;
}

RouteNavigator_State RouteNavigator_GetState(void)
{
    if (finished != 0U) return ROUTE_STATE_FINISHED;
    return (RouteNavigator_State)state;
}

RouteNavigator_Result RouteNavigator_GetResult(void)
{
    return (RouteNavigator_Result)reason;
}

uint32_t RouteNavigator_ConsumeEvents(void)
{
    uint32_t events = route_events;
    route_events = ROUTE_EVENT_NONE;
    return events;
}

void RouteNavigator_SendLogs(void)
{
    SendLogs(reason, state, accumulated_yaw);
    log_session_active = 0U;
}
#include <stdint.h>

#include "Application/Task1_AutoTrace.h"

#include "Hardware/Bluetooth.h"
#include "Hardware/CONTROL/GrayTrack.h"
#include "Hardware/CONTROL/HeadingControl.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/IMU.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define CONTROL_MS                    10U
#define LOG_MS                       100U
#define ROUTE_TIMEOUT_MS           20000U
#define IMU_READY_MS                1000U

#define AB_TARGET_DEG                  0.0f
#define CD_TARGET_DEG                180.0f
#define BC_SWITCH_PROGRESS_DEG       140.0f
#define C_ADVANCE_RPM                115.0f
#define C_ADVANCE_COUNT             9300L
#define C_ADVANCE_TIMEOUT_MS        1200U
#define C_ALIGN_DONE_DEG             178.0f
#define C_ALIGN_TIMEOUT_MS          1000U
#define C_BRAKE_MS                   120U
#define C_ALIGN_BASE_RPM              50.0f
#define DA_SWITCH_PROGRESS_DEG       320.0f
#define A_ADVANCE_RPM                115.0f
#define A_ADVANCE_COUNT             9300L
#define A_ADVANCE_TIMEOUT_MS        1200U
#define HEADING_BASE_RPM             115.0f
#define HEADING_KP                     3.0f
#define HEADING_KI                     0.0f
#define HEADING_KD                     3.0f
#define ADVANCE_HEADING_KP             2.0f
#define ADVANCE_HEADING_KI             0.0f
#define ADVANCE_HEADING_KD             1.0f
#define TARGET_SLEW_PER_YAW_FRAME      4.0f

#define LINE_DETECT_DELAY_MS          500U
#define LINE_DETECT_CONFIRM             3U
#define CAPTURE_BASE_RPM              70.0f
#define CAPTURE_TARGET_MIN_RPM        30.0f
#define CAPTURE_TARGET_MAX_RPM       110.0f
#define CAPTURE_CENTER_ERROR            1
#define CAPTURE_CENTER_CONFIRM           5U
#define CAPTURE_TIMEOUT_MS           1500U

#define LOG_MAX                       190U

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
    STATE_BRAKE_C = 10
};

enum
{
    STOP_LAP_COMPLETE = 1,
    STOP_EARLY_LOST = 2,
    STOP_ROUTE_TIMEOUT = 3,
    STOP_NO_IMU = 4,
    STOP_CAPTURE_TIMEOUT = 5
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
    LOG_EVENT_C_BRAKE_DONE = 9
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

static void SaveLog(uint32_t ms, uint8_t state, uint8_t event,
                    float yaw, float accum,
                    const GrayTrack_Output *gray,
                    const HeadingControl_Output *heading,
                    float left_target, float right_target)
{
    LogItem *item;
    int32_t left_count;
    int32_t right_count;

    if (log_count >= LOG_MAX) return;
    item = &logs[log_count++];

    item->ms = ms;
    item->state = state;
    item->event = event;
    item->raw = gray->raw;
    item->black = gray->black_mask;
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
    item->left_target10 = I16(left_target * 10.0f);
    item->right_target10 = I16(right_target * 10.0f);
    item->left_rpm10 = I16(SpeedPI_GetLeftRPM() * 10.0f);
    item->right_rpm10 = I16(SpeedPI_GetRightRPM() * 10.0f);
    item->left_pwm = (int16_t)SpeedPI_GetLeftPWM();
    item->right_pwm = (int16_t)SpeedPI_GetRightPWM();
}

static void SendLogs(uint8_t reason, uint8_t state, float accum)
{
    uint16_t i;

    SendLine("S");
    SendLine("PF,version,control_ms,log_ms,pulse_per_cycle,wheel_radius_mm,c_count,c_rpm,c_timeout_ms,a_count,a_rpm,a_timeout_ms,advance_kp_x100,advance_kd_x100,c_align_rpm,c_brake_ms");
    SendLine("P,4,10,100,14000,33,9300,115,1200,9300,115,1200,200,100,50,120");
    SendLine("M,event,0=NONE,1=C_ADV_ENTER,2=C_ADV_COUNT,3=C_ADV_TIMEOUT,4=C_ALIGN_YAW,5=C_ALIGN_TIMEOUT,6=A_ADV_ENTER,7=A_ADV_COUNT,8=A_ADV_TIMEOUT,9=C_BRAKE_DONE");
    SendLine("F,time_ms,state,event,raw,black,gray_error,left_count,right_count,advance_count,yaw_x100,accum_x10,heading_target_x100,heading_error_x100,correction_x100,left_target_x10,right_target_x10,left_rpm_x10,right_rpm_x10,left_pwm,right_pwm");

    for (i = 0U; i < log_count; i++)
    {
        LogItem *item = &logs[i];
        Bluetooth_SendString("D,");
        SendU32(item->ms);
        SendField(item->state);
        SendField(item->event);
        SendField(item->raw);
        SendField(item->black);
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
    SendU32(reason);
    SendField(state);
    SendField((int32_t)(accum * 100.0f));
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
    SendLine("E");
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
    *left_target =
        Clamp(CAPTURE_BASE_RPM + gray->correction_rpm,
              CAPTURE_TARGET_MIN_RPM,
              CAPTURE_TARGET_MAX_RPM);
    *right_target =
        Clamp(CAPTURE_BASE_RPM - gray->correction_rpm,
              CAPTURE_TARGET_MIN_RPM,
              CAPTURE_TARGET_MAX_RPM);
}

__attribute__((optnone)) void Task1_AutoTrace_Run(void)
{
    IMU_Data imu;
    GrayTrack_Output gray;
    HeadingControl_Output heading;
    uint32_t ms = 0U;
    uint32_t wait_ms = 0U;
    uint32_t next_log_ms = LOG_MS;
    uint32_t yaw_frames = 0U;
    uint32_t state_ms = 0U;
    float last_yaw = 0.0f;
    float accumulated_yaw = 0.0f;
    float heading_command = 0.0f;
    float advance_heading_target = 0.0f;
    float left_target = 0.0f;
    float right_target = 0.0f;
    uint8_t state = STATE_HEADING_AB;
    uint8_t reason = 0U;
    uint8_t line_confirm = 0U;
    uint8_t center_confirm = 0U;
    uint8_t log_event = LOG_EVENT_NONE;

    log_count = 0U;
    GrayTrack_Reset();
    SpeedPI_Reset();
    HeadingControl_Init();
    Encoder_ClearCount();
    Motor_Coast();

    do
    {
        IMU_GetData(&imu);
        if (imu.yaw_frame_count != 0U) break;
        delay_ms(CONTROL_MS);
        wait_ms += CONTROL_MS;
    } while (wait_ms < IMU_READY_MS);

    if (imu.yaw_frame_count == 0U)
    {
        reason = STOP_NO_IMU;
    }
    else
    {
        last_yaw = imu.yaw_deg;
        yaw_frames = imu.yaw_frame_count;
        Encoder_ClearCount();

        HeadingControl_SetGains(HEADING_KP, HEADING_KI, HEADING_KD);
        HeadingControl_SetTarget(AB_TARGET_DEG, HEADING_BASE_RPM);
        HeadingControl_Reset(0.0f);

        while ((reason == 0U) && (ms < ROUTE_TIMEOUT_MS))
        {
            uint8_t new_yaw = 0U;

            delay_ms(CONTROL_MS);
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
                    state_ms = 0U;
                    heading_command = CD_TARGET_DEG;
                    HeadingControl_SetGains(HEADING_KP, HEADING_KI,
                                            HEADING_KD);
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
                }
                HeadingControl_GetOutput(&heading);
                left_target = heading.left_target_rpm;
                right_target = heading.right_target_rpm;

                if (accumulated_yaw >= C_ALIGN_DONE_DEG)
                {
                    log_event = LOG_EVENT_C_ALIGN_YAW;
                    state = STATE_HEADING_CD;
                    state_ms = 0U;
                    line_confirm = 0U;
                    HeadingControl_SetTarget(heading_command,
                                             HEADING_BASE_RPM);
                    HeadingControl_GetOutput(&heading);
                    left_target = heading.left_target_rpm;
                    right_target = heading.right_target_rpm;
                }
                else if (state_ms >= C_ALIGN_TIMEOUT_MS)
                {
                    log_event = LOG_EVENT_C_ALIGN_TIMEOUT;
                    state = STATE_HEADING_CD;
                    state_ms = 0U;
                    line_confirm = 0U;
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
                    reason = STOP_LAP_COMPLETE;
                }
                else if (state_ms >= A_ADVANCE_TIMEOUT_MS)
                {
                    log_event = LOG_EVENT_A_ADVANCE_TIMEOUT;
                    reason = STOP_LAP_COMPLETE;
                }
            }

            HeadingControl_GetOutput(&heading);

            if ((reason == 0U) && (state != STATE_BRAKE_C))
            {
                SpeedPI_Update(left_target, right_target);
            }

            if (log_event != LOG_EVENT_NONE)
            {
                SaveLog(ms, state, log_event,
                        imu.yaw_deg, accumulated_yaw,
                        &gray, &heading, left_target, right_target);
                log_event = LOG_EVENT_NONE;
                if (ms >= next_log_ms)
                {
                    next_log_ms += LOG_MS;
                }
            }
            else if (ms >= next_log_ms)
            {
                SaveLog(ms, state, LOG_EVENT_NONE,
                        imu.yaw_deg, accumulated_yaw,
                        &gray, &heading, left_target, right_target);
                next_log_ms += LOG_MS;
            }
        }

        if (reason == 0U)
        {
            reason = STOP_ROUTE_TIMEOUT;
        }
    }

    GrayTrack_GetOutput(&gray);
    HeadingControl_GetOutput(&heading);
    SaveLog(ms, state, LOG_EVENT_NONE, imu.yaw_deg, accumulated_yaw,
            &gray, &heading, left_target, right_target);

    SpeedPI_Reset();
    Motor_Brake();
    delay_ms(300U);
    Motor_Coast();
    SendLogs(reason, state, accumulated_yaw);
}


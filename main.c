#include "Application/RobotPlatform.h"
#include "Application/Task1_AutoTrace.h"
#include "Application/Task3_LinkedOperation.h"
#include "Application/TaskBonus1_LaserTrace.h"
#include "Application/TaskBonusFourLap.h"
#include "Hardware/Bluetooth.h"
#include "Hardware/CONTROL/HeadingControl.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/IMU.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define TASK_MODE_TASK1_AUTO_TRACE       1U
#define TASK_MODE_TASK3_LINKED_OPERATION 3U
#define TASK_MODE_BONUS_FOUR_LAP         4U
#define TASK_MODE_BONUS1_LASER_TRACE     5U
#define TASK_MODE_LEFT_SPEED_20_RPM       6U
#define TASK_MODE_LEFT_SPEED_VARIABLE     7U
#define TASK_MODE_TIMING_IMU_DIAG         9U
#define TASK_MODE_BLUETOOTH_TX_TEST       8U
/* Change only this value to select the program started after power-on. */
#define SELECTED_TASK_MODE  TASK_MODE_LEFT_SPEED_20_RPM

#define TIMING_DIAG_DURATION_MS      10000U
#define TIMING_DIAG_EXPECTED_MS         10U
#define TIMING_DIAG_REPORT_MS        1000U

#define ANGLE_ENCODER_TEST_TOTAL_MS  18000U
#define ANGLE_ENCODER_TEST_LOG_MS      100U
#define ANGLE_ENCODER_BASE_RPM        25.0f
#define ANGLE_ENCODER_KP               1.2f
#define ANGLE_ENCODER_KI               0.0f
#define ANGLE_ENCODER_KD               0.2f
#define ANGLE_ENCODER_CORR_LIMIT_RPM  35.0f
#define ANGLE_ENCODER_TARGET_MIN_RPM   0.0f
#define ANGLE_ENCODER_TARGET_MAX_RPM  70.0f
#define OPEN_LOOP_SETTLE_MS          500U
#define OPEN_LOOP_MEASURE_MS        1500U
#define VOFA_PID_TEST_IDLE_MS    2000U
#define VOFA_PID_TEST_HOLD_MS    2000U
#define VOFA_PID_TEST_STOP_MS    2000U
#define VOFA_PID_TEST_LOG_MS       50U
#define VOFA_PID_FILTER_ALPHA       0.35f
#define VOFA_PID_FILTER_WINDOW      3U
#define LEFT_SPEED_TEST_IDLE_MS     2000U
#define LEFT_SPEED_TEST_HOLD_MS     4000U
#define LEFT_SPEED_TEST_STOP_MS     2000U
#define LEFT_SPEED_TEST_LOG_MS        50U
static void SendU32(uint32_t value)
{
    char digits[10];
    uint32_t count = 0U;

    do
    {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    while (count != 0U)
    {
        Bluetooth_SendByte((uint8_t)digits[--count]);
    }
}

static void SendFieldU32(uint32_t value)
{
    Bluetooth_SendByte((uint8_t)',');
    SendU32(value);
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

static void SendFieldI32(int32_t value)
{
    Bluetooth_SendByte((uint8_t)',');
    SendI32(value);
}

static int32_t ScaleFloat(float value, float scale)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value * scale + 0.5f);
    }
    return (int32_t)(value * scale - 0.5f);
}

static float YawDelta(float now, float old)
{
    float delta = now - old;

    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;

    return delta;
}
static void SendLine(const char *text)
{
    Bluetooth_SendString(text);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

static void SendTimingDiagResult(uint32_t elapsed_ms, uint32_t loops,
                                 uint32_t min_dt, uint32_t max_dt,
                                 uint32_t dt_lt_expected,
                                 uint32_t dt_expected,
                                 uint32_t dt_2x_expected,
                                 uint32_t dt_gt_2x_expected,
                                 uint32_t yaw_frames,
                                 uint32_t gyro_frames,
                                 uint32_t rx_bytes,
                                 uint32_t checksum_errors)
{
    SendLine("TD_HEADER,elapsed_ms,loops,min_dt_ms,max_dt_ms,dt_lt_10,dt_eq_10,dt_11_to_20,dt_gt_20,yaw_frames,gyro_frames,yaw_hz_x10,gyro_hz_x10,rx_bytes,checksum_errors");
    Bluetooth_SendString("TD");
    SendFieldU32(elapsed_ms);
    SendFieldU32(loops);
    SendFieldU32(min_dt);
    SendFieldU32(max_dt);
    SendFieldU32(dt_lt_expected);
    SendFieldU32(dt_expected);
    SendFieldU32(dt_2x_expected);
    SendFieldU32(dt_gt_2x_expected);
    SendFieldU32(yaw_frames);
    SendFieldU32(gyro_frames);
    SendFieldU32((elapsed_ms != 0U) ? ((yaw_frames * 10000U) / elapsed_ms) : 0U);
    SendFieldU32((elapsed_ms != 0U) ? ((gyro_frames * 10000U) / elapsed_ms) : 0U);
    SendFieldU32(rx_bytes);
    SendFieldU32(checksum_errors);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

typedef struct
{
    float sample[VOFA_PID_FILTER_WINDOW];
    uint8_t count;
    uint8_t index;
    uint8_t initialized;
    float filtered_rpm;
} VofaPidSpeedFilter;

static VofaPidSpeedFilter g_vofa_right_filter;
static float g_vofa_right_raw_rpm;

static float VofaPidMedian3(float a, float b, float c)
{
    float temp;

    if (a > b)
    {
        temp = a;
        a = b;
        b = temp;
    }
    if (b > c)
    {
        temp = b;
        b = c;
        c = temp;
    }
    if (a > b)
    {
        temp = a;
        a = b;
        b = temp;
    }

    return b;
}

static void VofaPidSpeedFilter_Reset(VofaPidSpeedFilter *filter)
{
    uint8_t i;

    for (i = 0U; i < VOFA_PID_FILTER_WINDOW; i++)
    {
        filter->sample[i] = 0.0f;
    }

    filter->count = 0U;
    filter->index = 0U;
    filter->initialized = 0U;
    filter->filtered_rpm = 0.0f;
    g_vofa_right_raw_rpm = 0.0f;
}

static float VofaPidSpeedFilter_Update(VofaPidSpeedFilter *filter, float raw_rpm)
{
    float median_rpm = raw_rpm;

    filter->sample[filter->index] = raw_rpm;
    filter->index++;
    if (filter->index >= VOFA_PID_FILTER_WINDOW)
    {
        filter->index = 0U;
    }
    if (filter->count < VOFA_PID_FILTER_WINDOW)
    {
        filter->count++;
    }

    if (filter->count >= VOFA_PID_FILTER_WINDOW)
    {
        median_rpm = VofaPidMedian3(filter->sample[0],
                                    filter->sample[1],
                                    filter->sample[2]);
    }

    if (filter->initialized == 0U)
    {
        filter->filtered_rpm = median_rpm;
        filter->initialized = 1U;
    }
    else
    {
        filter->filtered_rpm += VOFA_PID_FILTER_ALPHA *
                                (median_rpm - filter->filtered_rpm);
    }

    return filter->filtered_rpm;
}
typedef struct
{
    int16_t right_pwm;
    uint32_t duration_ms;
} VofaOpenLoopTestStep;
static void WaitControlTicksForMs(uint32_t duration_ms);

static void SendVofaOpenLoopFrame(int16_t right_pwm, float right_raw_rpm,
                                  float right_filtered_rpm, int32_t count_delta)
{
    Bluetooth_SendString("pid:");
    SendI32((int32_t)right_pwm);
    SendFieldI32(ScaleFloat(right_raw_rpm, 10.0f));
    SendFieldI32(ScaleFloat(right_filtered_rpm, 10.0f));
    SendFieldI32(count_delta);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

static void RunVofaOpenLoopTestStep(int16_t right_pwm, uint32_t duration_ms)
{
    uint32_t step_start = board_millis();
    uint32_t next_log_ms = 0U;
    uint32_t step_ms;
    float right_raw_rpm;
    float right_filtered_rpm;
    int32_t right_count;
    int32_t last_right_count;
    int32_t count_delta;

    VofaPidSpeedFilter_Reset(&g_vofa_right_filter);
    last_right_count = Encoder_GetRightCount();
    move(0, right_pwm);

    while ((board_millis() - step_start) < duration_ms)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        move(0, right_pwm);
        right_raw_rpm = Encoder_GetRightSpeed();
        right_filtered_rpm = VofaPidSpeedFilter_Update(&g_vofa_right_filter,
                                                       right_raw_rpm);
        g_vofa_right_raw_rpm = right_raw_rpm;
        right_count = Encoder_GetRightCount();
        count_delta = right_count - last_right_count;
        last_right_count = right_count;
        step_ms = board_millis() - step_start;

        if (step_ms >= next_log_ms)
        {
            SendVofaOpenLoopFrame(right_pwm, right_raw_rpm,
                                  right_filtered_rpm, count_delta);
            next_log_ms += VOFA_PID_TEST_LOG_MS;
        }
    }
}

static void RunVofaPidSpeedTest(void)
{
    static const VofaOpenLoopTestStep steps[] = {
        {0,  VOFA_PID_TEST_IDLE_MS},
        {32, VOFA_PID_TEST_HOLD_MS},
        {35, VOFA_PID_TEST_HOLD_MS},
        {38, VOFA_PID_TEST_HOLD_MS},
        {40, VOFA_PID_TEST_HOLD_MS},
        {45, VOFA_PID_TEST_HOLD_MS},
        {50, VOFA_PID_TEST_HOLD_MS},
        {60, VOFA_PID_TEST_HOLD_MS},
        {75, VOFA_PID_TEST_HOLD_MS},
        {90, VOFA_PID_TEST_HOLD_MS},
        {0,  VOFA_PID_TEST_STOP_MS}
    };
    uint32_t i;

    Motor_Brake();
    delay_ms(500U);
    Encoder_ClearCount();
    SpeedPI_Reset();
    VofaPidSpeedFilter_Reset(&g_vofa_right_filter);
    board_clear_control_ticks();

    for (i = 0U; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        RunVofaOpenLoopTestStep(steps[i].right_pwm, steps[i].duration_ms);
    }

    SpeedPI_Reset();
    VofaPidSpeedFilter_Reset(&g_vofa_right_filter);
    Motor_Brake();
    SendVofaOpenLoopFrame(0, 0.0f, 0.0f, 0);

    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}
static void WaitControlTicksForMs(uint32_t duration_ms)
{
    uint32_t start_ms = board_millis();

    while ((board_millis() - start_ms) < duration_ms)
    {
        while (board_consume_control_tick() == 0U)
        {
        }
    }
}

typedef struct
{
    float target_rpm;
    uint32_t duration_ms;
} LeftSpeedTestStep;

static void SendLeftSpeedLog(uint32_t elapsed_ms, float target_rpm)
{
    float actual_rpm = SpeedPI_GetLeftRPM();

    Bluetooth_SendString("LST");
    SendFieldU32(elapsed_ms);
    SendFieldI32(ScaleFloat(target_rpm, 10.0f));
    SendFieldI32(ScaleFloat(actual_rpm, 10.0f));
    SendFieldI32(ScaleFloat(target_rpm - actual_rpm, 10.0f));
    SendFieldI32(ScaleFloat(SpeedPI_GetLeftRawPWM(), 10.0f));
    SendFieldI32((int32_t)SpeedPI_GetLeftPWM());
    SendFieldI32(Encoder_GetLeftCount());
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

static void RunLeftSpeedTest(const LeftSpeedTestStep *steps, uint32_t step_count)
{
    uint32_t test_start;
    uint32_t step_start;
    uint32_t next_log_ms = 0U;
    uint32_t elapsed_ms;
    uint32_t i;

    Motor_Brake();
    delay_ms(500U);
    Encoder_ClearCount();
    SpeedPI_Reset();
    board_clear_control_ticks();
    SendLine("LST_HEADER,time_ms,target_rpm_x10,actual_rpm_x10,error_rpm_x10,raw_pwm_x10,final_pwm,left_count");
    SendLine("LST_BEGIN");
    test_start = board_millis();

    for (i = 0U; i < step_count; i++)
    {
        step_start = board_millis();
        while ((board_millis() - step_start) < steps[i].duration_ms)
        {
            while (board_consume_control_tick() == 0U)
            {
            }

            SpeedPI_UpdateLeftOnly(steps[i].target_rpm);
            elapsed_ms = board_millis() - test_start;
            if (elapsed_ms >= next_log_ms)
            {
                SendLeftSpeedLog(elapsed_ms, steps[i].target_rpm);
                next_log_ms += LEFT_SPEED_TEST_LOG_MS;
            }
        }
    }

    SpeedPI_Reset();
    Motor_Brake();
    SendLeftSpeedLog(board_millis() - test_start, 0.0f);
    SendLine("LST_END");

    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}

static void RunLeftSpeed20RpmTest(void)
{
    static const LeftSpeedTestStep steps[] = {
        {0.0f,  LEFT_SPEED_TEST_IDLE_MS},
        {20.0f, LEFT_SPEED_TEST_HOLD_MS},
        {0.0f,  LEFT_SPEED_TEST_STOP_MS}
    };

    RunLeftSpeedTest(steps, sizeof(steps) / sizeof(steps[0]));
}

static void RunLeftVariableSpeedTest(void)
{
    static const LeftSpeedTestStep steps[] = {
        {0.0f,  LEFT_SPEED_TEST_IDLE_MS},
        {20.0f, LEFT_SPEED_TEST_HOLD_MS},
        {40.0f, LEFT_SPEED_TEST_HOLD_MS},
        {60.0f, LEFT_SPEED_TEST_HOLD_MS},
        {40.0f, LEFT_SPEED_TEST_HOLD_MS},
        {20.0f, LEFT_SPEED_TEST_HOLD_MS},
        {0.0f,  LEFT_SPEED_TEST_STOP_MS}
    };

    RunLeftSpeedTest(steps, sizeof(steps) / sizeof(steps[0]));
}

static int32_t OpenLoopAvgRpmX10(int32_t delta_count, uint32_t measure_ms)
{
    float rpm_x10;

    rpm_x10 = ((float)delta_count * 600000.0f) /
              (PULSE_PER_CYCLE * (float)measure_ms);
    return ScaleFloat(rpm_x10, 1.0f);
}

static void SendOpenLoopHeader(void)
{
    SendLine("OLT_HEADER,mode,pwm,avg_l_x10,avg_r_x10,last_l_x10,last_r_x10,delta_l,delta_r");
}

static void RunOpenLoopStep(uint8_t mode, int pwm)
{
    int left_pwm = 0;
    int right_pwm = 0;
    int32_t left_start;
    int32_t right_start;
    int32_t left_delta;
    int32_t right_delta;

    if (mode == 1U)
    {
        left_pwm = pwm;
    }
    else if (mode == 2U)
    {
        right_pwm = pwm;
    }
    else
    {
        left_pwm = pwm;
        right_pwm = pwm;
    }

    move(left_pwm, right_pwm);
    WaitControlTicksForMs(OPEN_LOOP_SETTLE_MS);

    left_start = Encoder_GetLeftCount();
    right_start = Encoder_GetRightCount();
    WaitControlTicksForMs(OPEN_LOOP_MEASURE_MS);

    left_delta = Encoder_GetLeftCount() - left_start;
    right_delta = Encoder_GetRightCount() - right_start;

    Bluetooth_SendString("OLT");
    SendFieldU32((uint32_t)mode);
    SendFieldI32((int32_t)pwm);
    SendFieldI32(OpenLoopAvgRpmX10(left_delta, OPEN_LOOP_MEASURE_MS));
    SendFieldI32(OpenLoopAvgRpmX10(right_delta, OPEN_LOOP_MEASURE_MS));
    SendFieldI32(ScaleFloat(Encoder_GetLeftSpeed(), 10.0f));
    SendFieldI32(ScaleFloat(Encoder_GetRightSpeed(), 10.0f));
    SendFieldI32(left_delta);
    SendFieldI32(right_delta);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);

    move(0, 0);
    WaitControlTicksForMs(300U);
}

static void RunOpenLoopSpeedTest(void)
{
    static const int single_pwm[] = {30, 40, 50, 60, 80};
    static const int both_pwm[] = {30, 40, 50, 60, 80, 100, 130, 160, 200, 250, 300};
    uint32_t i;

    Motor_Brake();
    delay_ms(500U);
    Encoder_ClearCount();
    board_clear_control_ticks();

    SendLine("OLT_BEGIN");
    SendOpenLoopHeader();
    SendLine("OLT_MODE,1=LEFT_ONLY,2=RIGHT_ONLY,3=BOTH");

    for (i = 0U; i < (sizeof(single_pwm) / sizeof(single_pwm[0])); i++)
    {
        RunOpenLoopStep(1U, single_pwm[i]);
    }

    for (i = 0U; i < (sizeof(single_pwm) / sizeof(single_pwm[0])); i++)
    {
        RunOpenLoopStep(2U, single_pwm[i]);
    }

    for (i = 0U; i < (sizeof(both_pwm) / sizeof(both_pwm[0])); i++)
    {
        RunOpenLoopStep(3U, both_pwm[i]);
    }

    Motor_Brake();
    SendLine("OLT_END");

    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}
static float AngleEncoderTarget(uint32_t elapsed_ms)
{
    if (elapsed_ms < 3000U) return 0.0f;
    if (elapsed_ms < 7000U) return 30.0f;
    if (elapsed_ms < 11000U) return 0.0f;
    if (elapsed_ms < 15000U) return -30.0f;
    return 0.0f;
}

static void SendAngleEncoderHeader(void)
{
    SendLine("AET_HEADER,time_ms,target_yaw_x100,actual_yaw_x100,error_x100,corr_x100,left_target_x10,right_target_x10,left_rpm_x10,right_rpm_x10,left_count,right_count,left_pwm,right_pwm,yaw_frames,gyro_frames");
}

static void SendAngleEncoderLog(uint32_t elapsed_ms,
                                const HeadingControl_Output *heading,
                                const IMU_Data *imu_data)
{
    Bluetooth_SendString("AET");
    SendFieldU32(elapsed_ms);
    SendFieldI32(ScaleFloat(heading->target_yaw_deg, 100.0f));
    SendFieldI32(ScaleFloat(heading->actual_yaw_deg, 100.0f));
    SendFieldI32(ScaleFloat(heading->error_deg, 100.0f));
    SendFieldI32(ScaleFloat(heading->correction_rpm, 100.0f));
    SendFieldI32(ScaleFloat(heading->left_target_rpm, 10.0f));
    SendFieldI32(ScaleFloat(heading->right_target_rpm, 10.0f));
    SendFieldI32(ScaleFloat(SpeedPI_GetLeftRPM(), 10.0f));
    SendFieldI32(ScaleFloat(SpeedPI_GetRightRPM(), 10.0f));
    SendFieldI32(Encoder_GetLeftCount());
    SendFieldI32(Encoder_GetRightCount());
    SendFieldI32((int32_t)SpeedPI_GetLeftPWM());
    SendFieldI32((int32_t)SpeedPI_GetRightPWM());
    SendFieldU32(imu_data->yaw_frame_count);
    SendFieldU32(imu_data->gyro_frame_count);
    Bluetooth_SendByte(13U);
    Bluetooth_SendByte(10U);
}

static void RunAngleEncoderTest(void)
{
    IMU_Data imu_data;
    HeadingControl_Output heading;
    uint32_t start_ms;
    uint32_t elapsed_ms;
    uint32_t next_log_ms = 0U;
    uint32_t yaw_frames = 0U;
    float last_yaw = 0.0f;
    float accumulated_yaw = 0.0f;
    float target_yaw = 0.0f;

    Motor_Brake();
    SendLine("AET_BEGIN");

    do
    {
        IMU_GetData(&imu_data);
    } while (imu_data.yaw_frame_count == 0U);

    last_yaw = imu_data.yaw_deg;
    yaw_frames = imu_data.yaw_frame_count;
    Encoder_ClearCount();
    SpeedPI_Reset();
    HeadingControl_Init();
    HeadingControl_SetGains(ANGLE_ENCODER_KP,
                            ANGLE_ENCODER_KI,
                            ANGLE_ENCODER_KD);
    HeadingControl_SetOutputLimits(ANGLE_ENCODER_CORR_LIMIT_RPM,
                                   ANGLE_ENCODER_TARGET_MIN_RPM,
                                   ANGLE_ENCODER_TARGET_MAX_RPM);
    HeadingControl_SetTarget(0.0f, ANGLE_ENCODER_BASE_RPM);
    HeadingControl_Reset(0.0f);
    board_clear_control_ticks();
    start_ms = board_millis();
    SendAngleEncoderHeader();

    while ((board_millis() - start_ms) < ANGLE_ENCODER_TEST_TOTAL_MS)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        elapsed_ms = board_millis() - start_ms;
        IMU_GetData(&imu_data);
        if (imu_data.yaw_frame_count != yaw_frames)
        {
            accumulated_yaw += YawDelta(imu_data.yaw_deg, last_yaw);
            last_yaw = imu_data.yaw_deg;
            yaw_frames = imu_data.yaw_frame_count;
        }

        target_yaw = AngleEncoderTarget(elapsed_ms);
        HeadingControl_SetTarget(target_yaw, ANGLE_ENCODER_BASE_RPM);
        HeadingControl_Update(accumulated_yaw);
        HeadingControl_GetOutput(&heading);
        SpeedPI_Update(heading.left_target_rpm, heading.right_target_rpm);

        if (elapsed_ms >= next_log_ms)
        {
            SendAngleEncoderLog(elapsed_ms, &heading, &imu_data);
            next_log_ms += ANGLE_ENCODER_TEST_LOG_MS;
        }
    }

    SpeedPI_Reset();
    Motor_Brake();
    HeadingControl_GetOutput(&heading);
    IMU_GetData(&imu_data);
    SendAngleEncoderLog(board_millis() - start_ms, &heading, &imu_data);
    SendLine("AET_END");

    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}
static void RunBluetoothTxTest(void)
{
    uint32_t count = 0U;
    uint8_t rx;

    while (1)
    {
        Motor_Brake();
        Bluetooth_SendString("BT_TEST");
        SendFieldU32(count++);
        Bluetooth_SendByte(13U);
        Bluetooth_SendByte(10U);

        while (Bluetooth_ReadByte(&rx))
        {
            Bluetooth_SendString("BT_RX");
            SendFieldU32((uint32_t)rx);
            Bluetooth_SendByte(13U);
            Bluetooth_SendByte(10U);
        }

        delay_ms(500U);
    }
}
static void RunTimingImuDiagnostic(void)
{
    IMU_Data imu_start;
    IMU_Data imu_now;
    uint32_t start_ms;
    uint32_t last_ms;
    uint32_t now_ms;
    uint32_t dt_ms;
    uint32_t loops = 0U;
    uint32_t min_dt = 0xFFFFFFFFU;
    uint32_t max_dt = 0U;
    uint32_t dt_lt_expected = 0U;
    uint32_t dt_expected = 0U;
    uint32_t dt_2x_expected = 0U;
    uint32_t dt_gt_2x_expected = 0U;
    uint32_t elapsed_ms;
    uint32_t next_report_ms;
    uint32_t yaw_frames;
    uint32_t gyro_frames;
    uint32_t rx_bytes;
    uint32_t checksum_errors;

    Motor_Brake();
    SendLine("TD_BEGIN");

    IMU_GetData(&imu_start);
    start_ms = board_millis();
    while (board_millis() == start_ms)
    {
    }
    board_clear_control_ticks();

    start_ms = board_millis();
    last_ms = start_ms;
    next_report_ms = TIMING_DIAG_REPORT_MS;

    while ((board_millis() - start_ms) < TIMING_DIAG_DURATION_MS)
    {
        while (board_consume_control_tick() == 0U)
        {
        }
        now_ms = board_millis();
        dt_ms = now_ms - last_ms;
        last_ms = now_ms;

        if (dt_ms < min_dt) min_dt = dt_ms;
        if (dt_ms > max_dt) max_dt = dt_ms;

        if (dt_ms < TIMING_DIAG_EXPECTED_MS)
        {
            dt_lt_expected++;
        }
        else if (dt_ms == TIMING_DIAG_EXPECTED_MS)
        {
            dt_expected++;
        }
        else if (dt_ms <= (TIMING_DIAG_EXPECTED_MS * 2U))
        {
            dt_2x_expected++;
        }
        else
        {
            dt_gt_2x_expected++;
        }

        loops++;
        elapsed_ms = now_ms - start_ms;
        if (elapsed_ms >= next_report_ms)
        {
            IMU_GetData(&imu_now);
            Bluetooth_SendString("TD_ALIVE");
            SendFieldU32(elapsed_ms);
            SendFieldU32(loops);
            SendFieldU32(imu_now.yaw_frame_count - imu_start.yaw_frame_count);
            SendFieldU32(imu_now.gyro_frame_count - imu_start.gyro_frame_count);
            Bluetooth_SendByte(13U);
            Bluetooth_SendByte(10U);
            next_report_ms += TIMING_DIAG_REPORT_MS;
        }
    }

    elapsed_ms = board_millis() - start_ms;
    IMU_GetData(&imu_now);
    yaw_frames = imu_now.yaw_frame_count - imu_start.yaw_frame_count;
    gyro_frames = imu_now.gyro_frame_count - imu_start.gyro_frame_count;
    rx_bytes = imu_now.rx_byte_count - imu_start.rx_byte_count;
    checksum_errors = imu_now.checksum_error_count - imu_start.checksum_error_count;

    SendLine("TD_END");
    while (1)
    {
        SendTimingDiagResult(elapsed_ms, loops, min_dt, max_dt,
                             dt_lt_expected, dt_expected,
                             dt_2x_expected, dt_gt_2x_expected,
                             yaw_frames, gyro_frames,
                             rx_bytes, checksum_errors);
        delay_ms(TIMING_DIAG_REPORT_MS);
        Motor_Brake();
    }
}
int main(void)
{
#if (SELECTED_TASK_MODE == TASK_MODE_LEFT_SPEED_20_RPM) || \
    (SELECTED_TASK_MODE == TASK_MODE_LEFT_SPEED_VARIABLE)
    board_init();
    Bluetooth_Init();
    Bluetooth_SendString("BOOT,BOARD_OK\r\n");
    Motor_Init();
    Bluetooth_SendString("BOOT,MOTOR_OK\r\n");
    Encoder_Init();
    SpeedPI_Init();
    Bluetooth_SendString("BOOT,SPEED_OK\r\n");
#else
    RobotPlatform_Init();
    LaserRelay_Off();
#endif
    Motor_Brake();
    SendLine("BOOT,WAIT_1S");
    delay_ms(1000U);
    board_clear_control_ticks();
    SendLine("BOOT,START_TASK");

#if SELECTED_TASK_MODE == TASK_MODE_TASK1_AUTO_TRACE
    Task1_AutoTrace_Run();

#elif SELECTED_TASK_MODE == TASK_MODE_TASK3_LINKED_OPERATION
    Task3_LinkedOperation_Run();

#elif SELECTED_TASK_MODE == TASK_MODE_BONUS_FOUR_LAP
    RunVofaPidSpeedTest();

#elif SELECTED_TASK_MODE == TASK_MODE_BONUS1_LASER_TRACE
    TaskBonus1_LaserTrace_Run();

#elif SELECTED_TASK_MODE == TASK_MODE_LEFT_SPEED_20_RPM
    RunLeftSpeed20RpmTest();

#elif SELECTED_TASK_MODE == TASK_MODE_LEFT_SPEED_VARIABLE
    RunLeftVariableSpeedTest();

#elif SELECTED_TASK_MODE == TASK_MODE_BLUETOOTH_TX_TEST
    RunBluetoothTxTest();

#elif SELECTED_TASK_MODE == TASK_MODE_TIMING_IMU_DIAG
    RunTimingImuDiagnostic();
#else
#error "SELECTED_TASK_MODE is not implemented"
#endif

    while (1)
    {
        Motor_Coast();
        delay_ms(100U);
    }
}

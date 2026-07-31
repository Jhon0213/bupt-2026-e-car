#include "Application/RobotPlatform.h"
#include "Application/OledKeyTest.h"
#include "Application/Task1_AutoTrace.h"
#include "Application/Task3_LinkedOperation.h"
#include "Application/TaskBonus1_LaserTrace.h"
#include "Hardware/StarFlash.h"
#include "Hardware/CONTROL/HeadingControl.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/IMU.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

#define TASK_MODE_TASK1_AUTO_TRACE       1U
#define TASK_MODE_TASK3_LINKED_OPERATION 3U
#define TASK_MODE_BONUS1_LASER_TRACE     5U
#define TASK_MODE_HEADING_TUNING          6U
#define TASK_MODE_IMU_TEST                7U
#define TASK_MODE_LEFT_SPEED_TUNING       8U
#define TASK_MODE_STARFLASH_UART_TEST     9U
#define TASK_MODE_STRAIGHT_30RPM_TEST    10U
#define TASK_MODE_OLED_KEY_TEST          11U

/* Use TASK_MODE_STARFLASH_UART_TEST only for UART2 link diagnostics. */
#define SELECTED_TASK_MODE TASK_MODE_OLED_KEY_TEST

#define LEFT_SPEED_TEST_TARGET_RPM        30.0f
#define LEFT_SPEED_TEST_IDLE_MS         2000U
#define LEFT_SPEED_TEST_RUN_MS         15000U
#define LEFT_SPEED_TEST_STOP_MS         2000U
#define LEFT_SPEED_TEST_LOG_MS            20U

#define STRAIGHT_TEST_BASE_RPM            30.0f
#define STRAIGHT_TEST_KP                   3.0f
#define STRAIGHT_TEST_KI                   0.0f
#define STRAIGHT_TEST_KD                   0.06f
#define STRAIGHT_TEST_CORRECTION_LIMIT_RPM 10.0f
#define STRAIGHT_TEST_TARGET_MAX_RPM      60.0f
#define STRAIGHT_TEST_FIRST_RUN_MS      8000U
#define STRAIGHT_TEST_LEFT_TURN_TARGET_DEG   90.0f
#define STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG   0.0f
#define STRAIGHT_TEST_TURN_BASE_RPM       30.0f
#define STRAIGHT_TEST_TURN_CORRECTION_LIMIT_RPM 30.0f
#define STRAIGHT_TEST_TURN_DONE_ERROR_DEG  2.0f
#define STRAIGHT_TEST_TURN_SETTLE_TICKS   10U
#define STRAIGHT_TEST_TURN_TIMEOUT_MS   5000U
#define STRAIGHT_TEST_POST_TURN_RUN_MS  3000U
#define STRAIGHT_TEST_STOP_MS           2000U
#define STRAIGHT_TEST_LOG_MS              20U

#define HEADING_TEST_BASE_RPM             30.0f
#define HEADING_TEST_KP                    3.0f
#define HEADING_TEST_KI                    0.0f
#define HEADING_TEST_KD                    0.0f
#define HEADING_TEST_CORRECTION_LIMIT_RPM 10.0f
#define HEADING_TEST_IDLE_MS             2000U
#define HEADING_TEST_STRAIGHT_MS         2000U
#define HEADING_TEST_STOP_MS             2000U
#define HEADING_TEST_LOG_MS               100U
#define HEADING_TEST_MAX_SAMPLES          200U

typedef struct
{
    uint16_t elapsed_ms;
    int16_t target_yaw_x10;
    int16_t actual_yaw_x10;
    int16_t error_yaw_x10;
    int16_t correction_rpm_x10;
    int16_t left_target_rpm_x10;
    int16_t right_target_rpm_x10;
    int16_t left_rpm_x10;
    int16_t right_rpm_x10;
} HeadingTestSample;

static HeadingTestSample g_heading_test_log[HEADING_TEST_MAX_SAMPLES];
static uint32_t g_heading_test_log_count;

static void Debug_SendByte(uint8_t byte)
{
    StarFlash_SendByte(byte);
}

static void Debug_SendString(const char *text)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        Debug_SendByte((uint8_t)*text++);
    }
}

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
        Debug_SendByte((uint8_t)digits[--count]);
    }
}

static void SendI32(int32_t value)
{
    if (value < 0)
    {
        Debug_SendByte((uint8_t)'-');
        SendU32((uint32_t)(-(value + 1)) + 1U);
    }
    else
    {
        SendU32((uint32_t)value);
    }
}

static char HexDigit(uint8_t value)
{
    value &= 0x0FU;
    return (value < 10U) ?
        (char)('0' + value) :
        (char)('A' + value - 10U);
}

static void SendHex8(uint8_t value)
{
    Debug_SendByte((uint8_t)HexDigit((uint8_t)(value >> 4)));
    Debug_SendByte((uint8_t)HexDigit(value));
}
static int32_t ScaleFloat(float value, float scale)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value * scale + 0.5f);
    }
    return (int32_t)(value * scale - 0.5f);
}

static float IMUYawDegrees(int16_t raw)
{
    return ((float)raw * 180.0f) / 32768.0f;
}

static int32_t IMUYawX100(int16_t raw)
{
    return ((int32_t)raw * 18000L) / 32768L;
}

static int32_t IMUGyroZX100(int16_t raw)
{
    return ((int32_t)raw * 200000L) / 32768L;
}

static float YawDelta(float now, float previous)
{
    float delta = now - previous;

    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return delta;
}

static void RecordHeadingTestSample(uint32_t elapsed_ms,
                                    const HeadingControl_Output *heading)
{
    HeadingTestSample *sample;

    if (g_heading_test_log_count >= HEADING_TEST_MAX_SAMPLES)
    {
        return;
    }

    sample = &g_heading_test_log[g_heading_test_log_count++];
    sample->elapsed_ms = (uint16_t)elapsed_ms;
    sample->target_yaw_x10 = (int16_t)ScaleFloat(heading->target_yaw_deg, 10.0f);
    sample->actual_yaw_x10 = (int16_t)ScaleFloat(heading->actual_yaw_deg, 10.0f);
    sample->error_yaw_x10 = (int16_t)ScaleFloat(heading->error_deg, 10.0f);
    sample->correction_rpm_x10 = (int16_t)ScaleFloat(heading->correction_rpm, 10.0f);
    sample->left_target_rpm_x10 = (int16_t)ScaleFloat(heading->left_target_rpm, 10.0f);
    sample->right_target_rpm_x10 = (int16_t)ScaleFloat(heading->right_target_rpm, 10.0f);
    sample->left_rpm_x10 = (int16_t)ScaleFloat(SpeedPI_GetLeftRPM(), 10.0f);
    sample->right_rpm_x10 = (int16_t)ScaleFloat(SpeedPI_GetRightRPM(), 10.0f);
}

static void SendFixed1(int16_t value_x10)
{
    int32_t value = value_x10;

    if (value < 0)
    {
        Debug_SendByte((uint8_t)'-');
        value = -value;
    }
    SendU32((uint32_t)value / 10U);
    Debug_SendByte((uint8_t)'.');
    Debug_SendByte((uint8_t)('0' + ((uint32_t)value % 10U)));
}

static void SendHeadingTestSamples(void)
{
    uint32_t i;

    Debug_SendString("time,target_yaw,actual_yaw,error,correction,left_target,right_target,left_rpm,right_rpm\r\n");
    for (i = 0U; i < g_heading_test_log_count; i++)
    {
        SendU32(g_heading_test_log[i].elapsed_ms);
        Debug_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].target_yaw_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].actual_yaw_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].error_yaw_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].correction_rpm_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].left_target_rpm_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].right_target_rpm_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].left_rpm_x10);
        StarFlash_SendByte((uint8_t)',');
        SendFixed1(g_heading_test_log[i].right_rpm_x10);
        Debug_SendString("\r\n");
    }
}

static void RunHeadingControlPhase(float base_rpm,
                                   float target_yaw_deg,
                                   uint32_t duration_ms,
                                   uint32_t test_start,
                                   uint32_t *next_log_ms,
                                   float *last_yaw,
                                   float *accumulated_yaw,
                                   uint32_t *yaw_frame_count)
{
    IMU_Data imu;
    HeadingControl_Output heading;
    uint32_t phase_start = board_millis();
    uint32_t elapsed_ms;

    while ((board_millis() - phase_start) < duration_ms)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        IMU_GetData(&imu);
        if (imu.yaw_frame_count != *yaw_frame_count)
        {
            float yaw_deg = IMUYawDegrees(imu.yaw_raw);
            *accumulated_yaw += YawDelta(yaw_deg, *last_yaw);
            *last_yaw = yaw_deg;
            *yaw_frame_count = imu.yaw_frame_count;
        }

        HeadingControl_SetTarget(target_yaw_deg, base_rpm);
        HeadingControl_Update(*accumulated_yaw);
        HeadingControl_GetOutput(&heading);
        SpeedPI_Update(heading.left_target_rpm, heading.right_target_rpm);

        elapsed_ms = board_millis() - test_start;
        if (elapsed_ms >= *next_log_ms)
        {
            RecordHeadingTestSample(elapsed_ms, &heading);
            *next_log_ms += HEADING_TEST_LOG_MS;
        }
    }
}

static void RunIMUTest(void)
{
    IMU_Data imu;
    uint32_t start_ms = board_millis();
    uint32_t next_log_ms = 0U;

    Motor_Brake();
    while (1)
    {
        uint32_t elapsed_ms = board_millis() - start_ms;

        if (elapsed_ms >= next_log_ms)
        {
            IMU_GetData(&imu);
            Debug_SendString("imu:");
            SendU32(elapsed_ms);
            StarFlash_SendByte((uint8_t)',');
            SendI32(IMUYawX100(imu.yaw_raw));
            StarFlash_SendByte((uint8_t)',');
            SendI32(IMUGyroZX100(imu.gyro_z_raw));
            StarFlash_SendByte((uint8_t)',');
            SendU32(imu.yaw_frame_count);
            StarFlash_SendByte((uint8_t)',');
            SendU32(imu.gyro_frame_count);
            StarFlash_SendByte((uint8_t)',');
            SendU32(imu.rx_byte_count);
            StarFlash_SendByte((uint8_t)',');
            SendU32(imu.checksum_error_count);
            Debug_SendString("\r\n");
            next_log_ms += 100U;
        }
    }
}

static void SendVofaRightSpeedSample(uint32_t elapsed_ms, float target_rpm)
{
    float control_target_rpm = SpeedPI_GetRightTarget();
    float actual_rpm = SpeedPI_GetRightRPM();

    SendU32(elapsed_ms);
    StarFlash_SendByte((uint8_t)',');
    (void)target_rpm;
    SendFixed1((int16_t)ScaleFloat(control_target_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(actual_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(control_target_rpm - actual_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(Encoder_GetRightRawSpeed(), 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendI32(Encoder_GetRightDeltaCount());
    StarFlash_SendByte((uint8_t)',');
    SendI32(ScaleFloat(SpeedPI_GetRightRawPWM(), 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendI32(SpeedPI_GetRightPWM());
    Debug_SendString("\n");
}

static void SendVofaStraightSample(uint32_t elapsed_ms,
                                   uint32_t phase,
                                   float base_rpm,
                                   float target_yaw_deg,
                                   float actual_yaw_deg,
                                   float error_yaw_deg,
                                   float correction_rpm,
                                   float left_target_rpm,
                                   float right_target_rpm,
                                   uint32_t end_flag)
{
    SendU32(elapsed_ms);
    StarFlash_SendByte((uint8_t)',');
    SendU32(phase);
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(base_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(target_yaw_deg, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(actual_yaw_deg, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(error_yaw_deg, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(correction_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(left_target_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(right_target_rpm, 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(Encoder_GetLeftSpeed(), 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(Encoder_GetRightSpeed(), 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(Encoder_GetLeftRawSpeed(), 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendFixed1((int16_t)ScaleFloat(Encoder_GetRightRawSpeed(), 10.0f));
    StarFlash_SendByte((uint8_t)',');
    SendI32(Encoder_GetLeftDeltaCount());
    StarFlash_SendByte((uint8_t)',');
    SendI32(Encoder_GetRightDeltaCount());
    StarFlash_SendByte((uint8_t)',');
    SendI32(SpeedPI_GetLeftPWM());
    StarFlash_SendByte((uint8_t)',');
    SendI32(SpeedPI_GetRightPWM());
    StarFlash_SendByte((uint8_t)',');
    SendU32(end_flag);
    Debug_SendString("\n");
}

static void StraightTest_UpdateYaw(float *last_yaw,
                                   float *accumulated_yaw,
                                   uint32_t *yaw_frame_count)
{
    IMU_Data imu;

    IMU_GetData(&imu);
    if (imu.yaw_frame_count != *yaw_frame_count)
    {
        float yaw_deg = IMUYawDegrees(imu.yaw_raw);
        *accumulated_yaw += YawDelta(yaw_deg, *last_yaw);
        *last_yaw = yaw_deg;
        *yaw_frame_count = imu.yaw_frame_count;
    }
}

static float FloatAbs(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void StraightTest_RunTimedHeadingPhase(uint32_t phase,
                                              float target_yaw_deg,
                                              float base_rpm,
                                              uint32_t duration_ms,
                                              uint32_t test_start,
                                              uint32_t *next_log_ms,
                                              float *last_yaw,
                                              float *accumulated_yaw,
                                              uint32_t *yaw_frame_count)
{
    HeadingControl_Output heading;
    uint32_t phase_start = board_millis();
    uint32_t elapsed_ms;

    HeadingControl_SetOutputLimits(STRAIGHT_TEST_CORRECTION_LIMIT_RPM,
                                   0.0f,
                                   STRAIGHT_TEST_TARGET_MAX_RPM);
    HeadingControl_SetTarget(target_yaw_deg, base_rpm);
    HeadingControl_Reset(*accumulated_yaw);

    while ((board_millis() - phase_start) < duration_ms)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        elapsed_ms = board_millis() - test_start;
        StraightTest_UpdateYaw(last_yaw, accumulated_yaw, yaw_frame_count);
        HeadingControl_SetTarget(target_yaw_deg, base_rpm);
        HeadingControl_Update(*accumulated_yaw);
        HeadingControl_GetOutput(&heading);
        SpeedPI_Update(heading.left_target_rpm, heading.right_target_rpm);

        if (elapsed_ms >= *next_log_ms)
        {
            SendVofaStraightSample(elapsed_ms,
                                   phase,
                                   heading.base_rpm,
                                   heading.target_yaw_deg,
                                   heading.actual_yaw_deg,
                                   heading.error_deg,
                                   heading.correction_rpm,
                                   heading.left_target_rpm,
                                   heading.right_target_rpm,
                                   0U);
            *next_log_ms += STRAIGHT_TEST_LOG_MS;
        }
    }
}

static void StraightTest_RunTurnPhase(uint32_t phase,
                                      float target_yaw_deg,
                                      uint32_t test_start,
                                      uint32_t *next_log_ms,
                                      float *last_yaw,
                                      float *accumulated_yaw,
                                      uint32_t *yaw_frame_count)
{
    HeadingControl_Output heading;
    uint32_t phase_start = board_millis();
    uint32_t elapsed_ms;
    uint32_t settle_ticks = 0U;

    HeadingControl_SetOutputLimits(STRAIGHT_TEST_TURN_CORRECTION_LIMIT_RPM,
                                   0.0f,
                                   STRAIGHT_TEST_TARGET_MAX_RPM);
    HeadingControl_SetTarget(target_yaw_deg,
                             STRAIGHT_TEST_TURN_BASE_RPM);
    HeadingControl_Reset(*accumulated_yaw);

    while (((board_millis() - phase_start) < STRAIGHT_TEST_TURN_TIMEOUT_MS) &&
           (settle_ticks < STRAIGHT_TEST_TURN_SETTLE_TICKS))
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        elapsed_ms = board_millis() - test_start;
        StraightTest_UpdateYaw(last_yaw, accumulated_yaw, yaw_frame_count);
        HeadingControl_SetTarget(target_yaw_deg,
                                 STRAIGHT_TEST_TURN_BASE_RPM);
        HeadingControl_Update(*accumulated_yaw);
        HeadingControl_GetOutput(&heading);
        SpeedPI_Update(heading.left_target_rpm, heading.right_target_rpm);

        if (FloatAbs(heading.error_deg) <= STRAIGHT_TEST_TURN_DONE_ERROR_DEG)
        {
            settle_ticks++;
        }
        else
        {
            settle_ticks = 0U;
        }

        if (elapsed_ms >= *next_log_ms)
        {
            SendVofaStraightSample(elapsed_ms,
                                   phase,
                                   heading.base_rpm,
                                   heading.target_yaw_deg,
                                   heading.actual_yaw_deg,
                                   heading.error_deg,
                                   heading.correction_rpm,
                                   heading.left_target_rpm,
                                   heading.right_target_rpm,
                                   0U);
            *next_log_ms += STRAIGHT_TEST_LOG_MS;
        }
    }
}
static void StraightTest_RunStopPhase(uint32_t phase,
                                      uint32_t duration_ms,
                                      uint32_t test_start,
                                      uint32_t *next_log_ms,
                                      float *last_yaw,
                                      float *accumulated_yaw,
                                      uint32_t *yaw_frame_count)
{
    uint32_t phase_start = board_millis();
    uint32_t elapsed_ms;

    SpeedPI_Reset();
    Motor_Brake();
    while ((board_millis() - phase_start) < duration_ms)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        elapsed_ms = board_millis() - test_start;
        StraightTest_UpdateYaw(last_yaw, accumulated_yaw, yaw_frame_count);
        Motor_Brake();

        if (elapsed_ms >= *next_log_ms)
        {
            SendVofaStraightSample(elapsed_ms,
                                   phase,
                                   0.0f,
                                   STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG,
                                   *accumulated_yaw,
                                   STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG - *accumulated_yaw,
                                   0.0f,
                                   0.0f,
                                   0.0f,
                                   0U);
            *next_log_ms += STRAIGHT_TEST_LOG_MS;
        }
    }
}

static void RunStraight30rpmTest(void)
{
    IMU_Data imu;
    uint32_t test_start;
    uint32_t elapsed_ms;
    uint32_t next_log_ms = 0U;
    uint32_t yaw_frame_count;
    float last_yaw;
    float accumulated_yaw = 0.0f;

    Motor_Brake();
    delay_ms(500U);
    do
    {
        IMU_GetData(&imu);
    } while (imu.yaw_frame_count == 0U);

    last_yaw = IMUYawDegrees(imu.yaw_raw);
    yaw_frame_count = imu.yaw_frame_count;
    Encoder_ClearCount();
    SpeedPI_Reset();
    HeadingControl_SetGains(STRAIGHT_TEST_KP,
                            STRAIGHT_TEST_KI,
                            STRAIGHT_TEST_KD);
    board_clear_control_ticks();
    test_start = board_millis();

    StraightTest_RunTimedHeadingPhase(1U,
                                      0.0f,
                                      STRAIGHT_TEST_BASE_RPM,
                                      STRAIGHT_TEST_FIRST_RUN_MS,
                                      test_start,
                                      &next_log_ms,
                                      &last_yaw,
                                      &accumulated_yaw,
                                      &yaw_frame_count);
    StraightTest_RunTurnPhase(2U,
                              STRAIGHT_TEST_LEFT_TURN_TARGET_DEG,
                              test_start,
                              &next_log_ms,
                              &last_yaw,
                              &accumulated_yaw,
                              &yaw_frame_count);
    StraightTest_RunTimedHeadingPhase(3U,
                                      STRAIGHT_TEST_LEFT_TURN_TARGET_DEG,
                                      STRAIGHT_TEST_BASE_RPM,
                                      STRAIGHT_TEST_POST_TURN_RUN_MS,
                                      test_start,
                                      &next_log_ms,
                                      &last_yaw,
                                      &accumulated_yaw,
                                      &yaw_frame_count);
    StraightTest_RunTurnPhase(4U,
                              STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG,
                              test_start,
                              &next_log_ms,
                              &last_yaw,
                              &accumulated_yaw,
                              &yaw_frame_count);
    StraightTest_RunTimedHeadingPhase(5U,
                                      STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG,
                                      STRAIGHT_TEST_BASE_RPM,
                                      STRAIGHT_TEST_POST_TURN_RUN_MS,
                                      test_start,
                                      &next_log_ms,
                                      &last_yaw,
                                      &accumulated_yaw,
                                      &yaw_frame_count);
    StraightTest_RunStopPhase(6U,
                              STRAIGHT_TEST_STOP_MS,
                              test_start,
                              &next_log_ms,
                              &last_yaw,
                              &accumulated_yaw,
                              &yaw_frame_count);

    SpeedPI_Reset();
    Motor_Brake();
    elapsed_ms = board_millis() - test_start;
    SendVofaStraightSample(elapsed_ms,
                           6U,
                           0.0f,
                           STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG,
                           accumulated_yaw,
                           STRAIGHT_TEST_RIGHT_TURN_TARGET_DEG - accumulated_yaw,
                           0.0f,
                           0.0f,
                           0.0f,
                           1U);

    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}
static void RunLeftSpeedTuningTest(void)
{
    uint32_t test_start;
    uint32_t elapsed_ms;
    uint32_t next_log_ms = 0U;
    float target_rpm;

    Motor_Brake();
    SpeedPI_Reset();
    Encoder_ClearCount();
    board_clear_control_ticks();
    test_start = board_millis();

    while (1)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        elapsed_ms = board_millis() - test_start;

        if (elapsed_ms < LEFT_SPEED_TEST_IDLE_MS)
        {
            target_rpm = 0.0f;
            SpeedPI_Reset();
            Motor_Brake();
        }
        else if (elapsed_ms < (LEFT_SPEED_TEST_IDLE_MS + LEFT_SPEED_TEST_RUN_MS))
        {
            target_rpm = LEFT_SPEED_TEST_TARGET_RPM;
            SpeedPI_UpdateRightOnly(target_rpm);
        }
        else if (elapsed_ms < (LEFT_SPEED_TEST_IDLE_MS + LEFT_SPEED_TEST_RUN_MS + LEFT_SPEED_TEST_STOP_MS))
        {
            target_rpm = 0.0f;
            SpeedPI_Reset();
            Motor_Brake();
        }
        else
        {
            SpeedPI_Reset();
            Motor_Brake();
            while (1)
            {
                Motor_Brake();
                delay_ms(100U);
            }
        }

        if (elapsed_ms >= next_log_ms)
        {
            SendVofaRightSpeedSample(elapsed_ms, target_rpm);
            next_log_ms += LEFT_SPEED_TEST_LOG_MS;
        }
    }
}

static void RunStarFlashUartTest(void)
{
    uint32_t report_count = 0U;
    uint8_t byte;

    Motor_Brake();
    Debug_SendString("STARFLASH_TEST_BEGIN,115200\r\n");

    while (1)
    {
        while (StarFlash_ReadByte(&byte))
        {
            Debug_SendString("STARFLASH_RX,0x");
            SendHex8(byte);
            Debug_SendString(",ascii=");
            if ((byte >= 32U) && (byte <= 126U))
            {
                Debug_SendByte(byte);
            }
            else
            {
                Debug_SendByte((uint8_t)'.');
            }
            Debug_SendString("\r\n");
        }

        Debug_SendString("STARFLASH_ALIVE,");
        SendU32(report_count++);
        Debug_SendString(",rx=");
        SendU32(StarFlash_GetReceivedCount());
        Debug_SendString(",overflow=");
        SendU32(StarFlash_GetOverflowCount());
        Debug_SendString("\r\n");

        Motor_Brake();
        delay_ms(500U);
    }
}
static void RunHeadingTuningTest(void)
{
    IMU_Data imu;
    uint32_t test_start;
    uint32_t next_log_ms = 0U;
    uint32_t yaw_frame_count;
    float last_yaw;
    float accumulated_yaw = 0.0f;

    Motor_Brake();
    delay_ms(500U);
    do
    {
        IMU_GetData(&imu);
    } while (imu.yaw_frame_count == 0U);

    last_yaw = IMUYawDegrees(imu.yaw_raw);
    yaw_frame_count = imu.yaw_frame_count;
    Encoder_ClearCount();
    SpeedPI_Reset();
    HeadingControl_SetGains(HEADING_TEST_KP,
                            HEADING_TEST_KI,
                            HEADING_TEST_KD);
    HeadingControl_SetOutputLimits(HEADING_TEST_CORRECTION_LIMIT_RPM,
                                   0.0f,
                                   60.0f);
    HeadingControl_SetTarget(0.0f, 0.0f);
    HeadingControl_Reset(0.0f);
    board_clear_control_ticks();
    g_heading_test_log_count = 0U;
    test_start = board_millis();

    RunHeadingControlPhase(0.0f, 0.0f, HEADING_TEST_IDLE_MS,
                           test_start, &next_log_ms,
                           &last_yaw, &accumulated_yaw, &yaw_frame_count);
    RunHeadingControlPhase(HEADING_TEST_BASE_RPM, 0.0f,
                           HEADING_TEST_STRAIGHT_MS,
                           test_start, &next_log_ms,
                           &last_yaw, &accumulated_yaw, &yaw_frame_count);
    RunHeadingControlPhase(0.0f, 0.0f, HEADING_TEST_STOP_MS,
                           test_start, &next_log_ms,
                           &last_yaw, &accumulated_yaw, &yaw_frame_count);

    SpeedPI_Reset();
    Motor_Brake();
    SendHeadingTestSamples();

    while (1)
    {
        Motor_Brake();
        delay_ms(100U);
    }
}

int main(void)
{
#if SELECTED_TASK_MODE == TASK_MODE_HEADING_TUNING
    RobotPlatform_Init();
    LaserRelay_Off();
#else
    RobotPlatform_Init();
    LaserRelay_Off();
#endif

    Motor_Brake();
    delay_ms(1000U);
#if (SELECTED_TASK_MODE != TASK_MODE_LEFT_SPEED_TUNING) && \
    (SELECTED_TASK_MODE != TASK_MODE_STRAIGHT_30RPM_TEST) && \
    (SELECTED_TASK_MODE != TASK_MODE_OLED_KEY_TEST)
    Debug_SendString("STARFLASH_UART2_READY,115200\r\n");
    delay_ms(500U);
    Debug_SendString("STARFLASH_UART2_READY,115200\r\n");
    delay_ms(500U);
    Debug_SendString("STARFLASH_UART2_READY,115200\r\n");
#endif
    board_clear_control_ticks();

#if SELECTED_TASK_MODE == TASK_MODE_TASK1_AUTO_TRACE
    Task1_AutoTrace_Run();
#elif SELECTED_TASK_MODE == TASK_MODE_TASK3_LINKED_OPERATION
    Task3_LinkedOperation_Run();
#elif SELECTED_TASK_MODE == TASK_MODE_BONUS1_LASER_TRACE
    TaskBonus1_LaserTrace_Run();
#elif SELECTED_TASK_MODE == TASK_MODE_HEADING_TUNING
    RunHeadingTuningTest();
#elif SELECTED_TASK_MODE == TASK_MODE_IMU_TEST
    RunIMUTest();
#elif SELECTED_TASK_MODE == TASK_MODE_LEFT_SPEED_TUNING
    RunLeftSpeedTuningTest();
#elif SELECTED_TASK_MODE == TASK_MODE_STRAIGHT_30RPM_TEST
    RunStraight30rpmTest();
#elif SELECTED_TASK_MODE == TASK_MODE_OLED_KEY_TEST
    OledKeyTest_Run();
#elif SELECTED_TASK_MODE == TASK_MODE_STARFLASH_UART_TEST
    RunStarFlashUartTest();
#else
#error "SELECTED_TASK_MODE is not implemented"
#endif

    while (1)
    {
        Motor_Coast();
        delay_ms(100U);
    }
}

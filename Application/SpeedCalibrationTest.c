#include "SpeedCalibrationTest.h"
#include "Application/BuildConfig.h"

#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/Motor.h"
#include "Hardware/StarFlash.h"
#include "Public/Board/board.h"

#include <stdint.h>

#ifndef SPEED_CAL_SELECTED_CASE
#define SPEED_CAL_SELECTED_CASE SPEED_CAL_BOTH_EQUAL
#endif

#define SPEED_CAL_CONTROL_PERIOD_MS       10U
#define SPEED_CAL_VOFA_PERIOD_MS          50U
#define SPEED_CAL_STOP_HOLD_MS           500U
#define SPEED_CAL_PREP_HOLD_MS           200U

#define SPEED_CAL_OPEN_IDLE_MS           500U
#define SPEED_CAL_OPEN_STEP_MS          1500U
#define SPEED_CAL_OPEN_STOP_MS           500U
#define SPEED_CAL_OPEN_STEP_COUNT          7U
#define SPEED_CAL_OPEN_TOTAL_MS        (SPEED_CAL_OPEN_IDLE_MS + \
                                        (SPEED_CAL_OPEN_STEP_MS * SPEED_CAL_OPEN_STEP_COUNT) + \
                                        SPEED_CAL_OPEN_STOP_MS)

#define SPEED_CAL_CLOSED_IDLE_MS         500U
#define SPEED_CAL_CLOSED_RAMP_UP_MS      600U
#define SPEED_CAL_CLOSED_HOLD_MS        1500U
#define SPEED_CAL_CLOSED_RAMP_DOWN_MS    600U
#define SPEED_CAL_CLOSED_STOP_MS         500U
#define SPEED_CAL_CLOSED_TOTAL_MS       (SPEED_CAL_CLOSED_IDLE_MS + \
                                         SPEED_CAL_CLOSED_RAMP_UP_MS + \
                                         SPEED_CAL_CLOSED_HOLD_MS + \
                                         SPEED_CAL_CLOSED_RAMP_DOWN_MS + \
                                         SPEED_CAL_CLOSED_STOP_MS)
#define SPEED_CAL_CLOSED_TARGET_RPM     100.0f

typedef enum
{
    SPEED_CAL_PHASE_IDLE = 0,
    SPEED_CAL_PHASE_OPEN_PWM_40,
    SPEED_CAL_PHASE_OPEN_PWM_60,
    SPEED_CAL_PHASE_OPEN_PWM_80,
    SPEED_CAL_PHASE_OPEN_PWM_100,
    SPEED_CAL_PHASE_OPEN_PWM_120,
    SPEED_CAL_PHASE_OPEN_PWM_140,
    SPEED_CAL_PHASE_OPEN_PWM_160,
    SPEED_CAL_PHASE_STOP,
    SPEED_CAL_PHASE_CLOSED_RAMP_UP,
    SPEED_CAL_PHASE_CLOSED_HOLD,
    SPEED_CAL_PHASE_CLOSED_RAMP_DOWN,
    SPEED_CAL_PHASE_UNSUPPORTED = 99
} SpeedCalibrationPhase;

typedef struct
{
    uint32_t elapsed_ms;
    int32_t test_case;
    int32_t phase;
    int32_t command_pwm;
    int32_t command_target_rpm_x10;
    int32_t control_target_rpm_x10;
    int32_t active_raw_rpm_x10;
    int32_t active_filtered_rpm_x10;
    int32_t active_error_rpm_x10;
    int32_t active_pwm;
    int32_t active_raw_pwm_x10;
    int32_t active_feedforward_pwm_x10;
    int32_t active_p_term_x10;
    int32_t active_integral_term_x10;
    int32_t active_encoder_delta;
    int32_t active_encoder_count;
    int32_t control_late_count;
    int32_t vofa_tx_count;
} SpeedCalibrationVofaFrame;

static uint32_t g_speed_cal_control_late_count;
static uint32_t g_speed_cal_vofa_tx_count;

static const uint16_t g_speed_cal_open_pwm[SPEED_CAL_OPEN_STEP_COUNT] =
{
    40U, 60U, 80U, 100U, 120U, 140U, 160U
};

static SpeedCalibrationTestCase SpeedCalibration_GetSelectedCase(void)
{
    return (SpeedCalibrationTestCase)SPEED_CAL_SELECTED_CASE;
}

static uint8_t SpeedCalibration_IsLeftCase(SpeedCalibrationTestCase test_case)
{
    return ((test_case == SPEED_CAL_LEFT_OPEN_LOOP) ||
            (test_case == SPEED_CAL_LEFT_CLOSED_LOOP)) ? 1U : 0U;
}

static uint8_t SpeedCalibration_IsBothCase(SpeedCalibrationTestCase test_case)
{
    return (test_case == SPEED_CAL_BOTH_EQUAL) ? 1U : 0U;
}

static uint8_t SpeedCalibration_IsOpenCase(SpeedCalibrationTestCase test_case)
{
    return ((test_case == SPEED_CAL_RIGHT_OPEN_LOOP) ||
            (test_case == SPEED_CAL_LEFT_OPEN_LOOP)) ? 1U : 0U;
}

static uint8_t SpeedCalibration_IsClosedCase(SpeedCalibrationTestCase test_case)
{
    return ((test_case == SPEED_CAL_RIGHT_CLOSED_LOOP) ||
            (test_case == SPEED_CAL_LEFT_CLOSED_LOOP)) ? 1U : 0U;
}

static int32_t SpeedCalibration_ScaleX10(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value * 10.0f + 0.5f);
    }
    return (int32_t)(value * 10.0f - 0.5f);
}

static float SpeedCalibration_ClampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void SpeedCalibration_SendByte(uint8_t byte)
{
    StarFlash_SendByte(byte);
}

static void SpeedCalibration_SendU32(uint32_t value)
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
        SpeedCalibration_SendByte((uint8_t)digits[--count]);
    }
}

static void SpeedCalibration_SendI32(int32_t value)
{
    if (value < 0)
    {
        SpeedCalibration_SendByte((uint8_t)'-');
        SpeedCalibration_SendU32((uint32_t)(-(value + 1)) + 1U);
    }
    else
    {
        SpeedCalibration_SendU32((uint32_t)value);
    }
}

static void SpeedCalibration_SendComma(void)
{
    SpeedCalibration_SendByte((uint8_t)',');
}

static uint32_t SpeedCalibration_GetDurationMs(SpeedCalibrationTestCase test_case)
{
    if ((SpeedCalibration_IsClosedCase(test_case) != 0U) ||
        (SpeedCalibration_IsBothCase(test_case) != 0U))
    {
        return SPEED_CAL_CLOSED_TOTAL_MS;
    }
    if (SpeedCalibration_IsOpenCase(test_case) != 0U)
    {
        return SPEED_CAL_OPEN_TOTAL_MS;
    }
    return 0U;
}

static int32_t SpeedCalibration_GetOpenCommandPwm(uint32_t elapsed_ms)
{
    uint32_t step_index;

    if (elapsed_ms < SPEED_CAL_OPEN_IDLE_MS)
    {
        return 0;
    }

    elapsed_ms -= SPEED_CAL_OPEN_IDLE_MS;
    step_index = elapsed_ms / SPEED_CAL_OPEN_STEP_MS;
    if (step_index < SPEED_CAL_OPEN_STEP_COUNT)
    {
        return (int32_t)g_speed_cal_open_pwm[step_index];
    }

    return 0;
}

static int32_t SpeedCalibration_GetOpenPhase(uint32_t elapsed_ms)
{
    uint32_t step_index;

    if (elapsed_ms < SPEED_CAL_OPEN_IDLE_MS)
    {
        return SPEED_CAL_PHASE_IDLE;
    }

    elapsed_ms -= SPEED_CAL_OPEN_IDLE_MS;
    step_index = elapsed_ms / SPEED_CAL_OPEN_STEP_MS;
    if (step_index < SPEED_CAL_OPEN_STEP_COUNT)
    {
        return (int32_t)(SPEED_CAL_PHASE_OPEN_PWM_40 + step_index);
    }

    return SPEED_CAL_PHASE_STOP;
}

static float SpeedCalibration_GetClosedCommandTarget(uint32_t elapsed_ms)
{
    uint32_t phase_ms;
    float target_rpm;

    if (elapsed_ms < SPEED_CAL_CLOSED_IDLE_MS)
    {
        return 0.0f;
    }

    phase_ms = elapsed_ms - SPEED_CAL_CLOSED_IDLE_MS;
    if (phase_ms < SPEED_CAL_CLOSED_RAMP_UP_MS)
    {
        target_rpm = (SPEED_CAL_CLOSED_TARGET_RPM * (float)phase_ms) /
                     (float)SPEED_CAL_CLOSED_RAMP_UP_MS;
        return SpeedCalibration_ClampFloat(target_rpm, 0.0f, SPEED_CAL_CLOSED_TARGET_RPM);
    }

    phase_ms -= SPEED_CAL_CLOSED_RAMP_UP_MS;
    if (phase_ms < SPEED_CAL_CLOSED_HOLD_MS)
    {
        return SPEED_CAL_CLOSED_TARGET_RPM;
    }

    phase_ms -= SPEED_CAL_CLOSED_HOLD_MS;
    if (phase_ms < SPEED_CAL_CLOSED_RAMP_DOWN_MS)
    {
        target_rpm = SPEED_CAL_CLOSED_TARGET_RPM -
                     ((SPEED_CAL_CLOSED_TARGET_RPM * (float)phase_ms) /
                      (float)SPEED_CAL_CLOSED_RAMP_DOWN_MS);
        return SpeedCalibration_ClampFloat(target_rpm, 0.0f, SPEED_CAL_CLOSED_TARGET_RPM);
    }

    return 0.0f;
}

static int32_t SpeedCalibration_GetClosedPhase(uint32_t elapsed_ms)
{
    uint32_t phase_ms;

    if (elapsed_ms < SPEED_CAL_CLOSED_IDLE_MS)
    {
        return SPEED_CAL_PHASE_IDLE;
    }

    phase_ms = elapsed_ms - SPEED_CAL_CLOSED_IDLE_MS;
    if (phase_ms < SPEED_CAL_CLOSED_RAMP_UP_MS)
    {
        return SPEED_CAL_PHASE_CLOSED_RAMP_UP;
    }

    phase_ms -= SPEED_CAL_CLOSED_RAMP_UP_MS;
    if (phase_ms < SPEED_CAL_CLOSED_HOLD_MS)
    {
        return SPEED_CAL_PHASE_CLOSED_HOLD;
    }

    phase_ms -= SPEED_CAL_CLOSED_HOLD_MS;
    if (phase_ms < SPEED_CAL_CLOSED_RAMP_DOWN_MS)
    {
        return SPEED_CAL_PHASE_CLOSED_RAMP_DOWN;
    }

    return SPEED_CAL_PHASE_STOP;
}

static void SpeedCalibration_SendVofaFrame(const SpeedCalibrationVofaFrame *frame)
{
#if !SPEED_CAL_TELEMETRY_ENABLE
    (void)frame;
    return;
#else
    SpeedCalibration_SendU32(frame->elapsed_ms);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->test_case);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->phase);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->command_pwm);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->command_target_rpm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->control_target_rpm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_raw_rpm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_filtered_rpm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_error_rpm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_pwm);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_raw_pwm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_feedforward_pwm_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_p_term_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_integral_term_x10);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_encoder_delta);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->active_encoder_count);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->control_late_count);
    SpeedCalibration_SendComma();
    SpeedCalibration_SendI32(frame->vofa_tx_count);
    SpeedCalibration_SendByte((uint8_t)'\r');
    SpeedCalibration_SendByte((uint8_t)'\n');
    g_speed_cal_vofa_tx_count++;
#endif
}

static void SpeedCalibration_FillCommonFrame(SpeedCalibrationVofaFrame *frame,
                                             SpeedCalibrationTestCase test_case,
                                             uint32_t elapsed_ms)
{
    frame->elapsed_ms = elapsed_ms;
    frame->test_case = (int32_t)test_case;

    if (SpeedCalibration_IsLeftCase(test_case) != 0U)
    {
        frame->active_raw_rpm_x10 = SpeedCalibration_ScaleX10(Encoder_GetLeftRawSpeed());
        frame->active_filtered_rpm_x10 = SpeedCalibration_ScaleX10(Encoder_GetLeftSpeed());
        frame->active_encoder_delta = Encoder_GetLeftDeltaCount();
        frame->active_encoder_count = Encoder_GetLeftCount();
    }
    else
    {
        frame->active_raw_rpm_x10 = SpeedCalibration_ScaleX10(Encoder_GetRightRawSpeed());
        frame->active_filtered_rpm_x10 = SpeedCalibration_ScaleX10(Encoder_GetRightSpeed());
        frame->active_encoder_delta = Encoder_GetRightDeltaCount();
        frame->active_encoder_count = Encoder_GetRightCount();
    }

    frame->control_late_count = (int32_t)g_speed_cal_control_late_count;
    frame->vofa_tx_count = (int32_t)g_speed_cal_vofa_tx_count;
}

static void SpeedCalibration_FillBothEqualFrame(SpeedCalibrationVofaFrame *frame,
                                                uint32_t elapsed_ms,
                                                int32_t phase,
                                                float common_target_rpm,
                                                const SpeedPI_CalibrationSample *left_sample,
                                                const SpeedPI_CalibrationSample *right_sample)
{
    int32_t left_filtered_rpm_x10;
    int32_t right_filtered_rpm_x10;
    int32_t left_count;
    int32_t right_count;

    frame->elapsed_ms = elapsed_ms;
    frame->test_case = (int32_t)SPEED_CAL_BOTH_EQUAL;
    frame->phase = phase;
    frame->command_pwm = SpeedCalibration_ScaleX10(common_target_rpm);

    if ((left_sample != 0) && (right_sample != 0))
    {
        left_filtered_rpm_x10 = SpeedCalibration_ScaleX10(left_sample->actual_rpm);
        right_filtered_rpm_x10 = SpeedCalibration_ScaleX10(right_sample->actual_rpm);
        frame->command_target_rpm_x10 = SpeedCalibration_ScaleX10(left_sample->control_target_rpm);
        frame->control_target_rpm_x10 = SpeedCalibration_ScaleX10(right_sample->control_target_rpm);
        frame->active_pwm = left_sample->pwm;
        frame->active_raw_pwm_x10 = right_sample->pwm;
        frame->active_feedforward_pwm_x10 = SpeedCalibration_ScaleX10(left_sample->integral_term);
        frame->active_p_term_x10 = SpeedCalibration_ScaleX10(right_sample->integral_term);
    }
    else
    {
        left_filtered_rpm_x10 = SpeedCalibration_ScaleX10(Encoder_GetLeftSpeed());
        right_filtered_rpm_x10 = SpeedCalibration_ScaleX10(Encoder_GetRightSpeed());
        frame->command_target_rpm_x10 = 0;
        frame->control_target_rpm_x10 = 0;
        frame->active_pwm = 0;
        frame->active_raw_pwm_x10 = 0;
        frame->active_feedforward_pwm_x10 = 0;
        frame->active_p_term_x10 = 0;
    }

    left_count = Encoder_GetLeftCount();
    right_count = Encoder_GetRightCount();
    frame->active_raw_rpm_x10 = left_filtered_rpm_x10;
    frame->active_filtered_rpm_x10 = right_filtered_rpm_x10;
    frame->active_error_rpm_x10 = left_filtered_rpm_x10 - right_filtered_rpm_x10;
    frame->active_integral_term_x10 = Encoder_GetLeftDeltaCount();
    frame->active_encoder_delta = Encoder_GetRightDeltaCount();
    frame->active_encoder_count = left_count - right_count;
    frame->control_late_count = (int32_t)g_speed_cal_control_late_count;
    frame->vofa_tx_count = (int32_t)g_speed_cal_vofa_tx_count;
}

static void SpeedCalibration_ControlBothEqual(uint32_t elapsed_ms,
                                              SpeedCalibrationVofaFrame *frame)
{
    SpeedPI_CalibrationSample left_sample;
    SpeedPI_CalibrationSample right_sample;
    float common_target_rpm = SpeedCalibration_GetClosedCommandTarget(elapsed_ms);

    SpeedPI_UpdateBothCalibrationDirect(common_target_rpm,
                                        common_target_rpm,
                                        &left_sample,
                                        &right_sample);
    SpeedCalibration_FillBothEqualFrame(frame,
                                        elapsed_ms,
                                        SpeedCalibration_GetClosedPhase(elapsed_ms),
                                        common_target_rpm,
                                        &left_sample,
                                        &right_sample);
}
static void SpeedCalibration_ControlOpenLoop(SpeedCalibrationTestCase test_case,
                                             uint32_t elapsed_ms,
                                             SpeedCalibrationVofaFrame *frame)
{
    int32_t command_pwm = SpeedCalibration_GetOpenCommandPwm(elapsed_ms);

    if (SpeedCalibration_IsLeftCase(test_case) != 0U)
    {
        SpeedPI_ResetRightOnly();
        move((int)command_pwm, 0);
    }
    else
    {
        SpeedPI_ResetLeftOnly();
        move(0, (int)command_pwm);
    }

    SpeedCalibration_FillCommonFrame(frame, test_case, elapsed_ms);
    frame->phase = SpeedCalibration_GetOpenPhase(elapsed_ms);
    frame->command_pwm = command_pwm;
    frame->command_target_rpm_x10 = 0;
    frame->control_target_rpm_x10 = 0;
    frame->active_error_rpm_x10 = 0;
    frame->active_pwm = command_pwm;
    frame->active_raw_pwm_x10 = 0;
    frame->active_feedforward_pwm_x10 = 0;
    frame->active_p_term_x10 = 0;
    frame->active_integral_term_x10 = 0;
}

static void SpeedCalibration_ControlClosedLoop(SpeedCalibrationTestCase test_case,
                                               uint32_t elapsed_ms,
                                               SpeedCalibrationVofaFrame *frame)
{
    SpeedPI_CalibrationSample pi_sample;
    float command_target_rpm = SpeedCalibration_GetClosedCommandTarget(elapsed_ms);

    if (SpeedCalibration_IsLeftCase(test_case) != 0U)
    {
        SpeedPI_UpdateLeftCalibrationDirect(command_target_rpm, &pi_sample);
    }
    else
    {
        SpeedPI_UpdateRightCalibrationDirect(command_target_rpm, &pi_sample);
    }

    SpeedCalibration_FillCommonFrame(frame, test_case, elapsed_ms);
    frame->phase = SpeedCalibration_GetClosedPhase(elapsed_ms);
    frame->command_pwm = 0;
    frame->command_target_rpm_x10 = SpeedCalibration_ScaleX10(command_target_rpm);
    frame->control_target_rpm_x10 = SpeedCalibration_ScaleX10(pi_sample.control_target_rpm);
    frame->active_filtered_rpm_x10 = SpeedCalibration_ScaleX10(pi_sample.actual_rpm);
    frame->active_error_rpm_x10 = SpeedCalibration_ScaleX10(pi_sample.error_rpm);
    frame->active_pwm = pi_sample.pwm;
    frame->active_raw_pwm_x10 = SpeedCalibration_ScaleX10(pi_sample.raw_pwm);
    frame->active_feedforward_pwm_x10 = SpeedCalibration_ScaleX10(pi_sample.feedforward_pwm);
    frame->active_p_term_x10 = SpeedCalibration_ScaleX10(pi_sample.p_term);
    frame->active_integral_term_x10 = SpeedCalibration_ScaleX10(pi_sample.integral_term);
}

static void SpeedCalibration_ControlStop(SpeedCalibrationTestCase test_case,
                                         uint32_t elapsed_ms,
                                         SpeedCalibrationVofaFrame *frame)
{
    SpeedPI_Reset();
    Motor_Coast();

    if (SpeedCalibration_IsBothCase(test_case) != 0U)
    {
        SpeedCalibration_FillBothEqualFrame(frame,
                                            elapsed_ms,
                                            SPEED_CAL_PHASE_STOP,
                                            0.0f,
                                            0,
                                            0);
        return;
    }

    SpeedCalibration_FillCommonFrame(frame, test_case, elapsed_ms);
    frame->phase = SPEED_CAL_PHASE_STOP;
    frame->command_pwm = 0;
    frame->command_target_rpm_x10 = 0;
    frame->control_target_rpm_x10 = 0;
    frame->active_error_rpm_x10 = 0 - frame->active_filtered_rpm_x10;
    frame->active_pwm = 0;
    frame->active_raw_pwm_x10 = 0;
    frame->active_feedforward_pwm_x10 = 0;
    frame->active_p_term_x10 = 0;
    frame->active_integral_term_x10 = 0;
}

static void SpeedCalibration_ControlUnsupported(SpeedCalibrationTestCase test_case,
                                                SpeedCalibrationVofaFrame *frame)
{
    SpeedPI_Reset();
    Motor_Coast();

    SpeedCalibration_FillCommonFrame(frame, test_case, 0U);
    frame->phase = SPEED_CAL_PHASE_UNSUPPORTED;
    frame->command_pwm = 0;
    frame->command_target_rpm_x10 = 0;
    frame->control_target_rpm_x10 = 0;
    frame->active_error_rpm_x10 = 0;
    frame->active_pwm = 0;
    frame->active_raw_pwm_x10 = 0;
    frame->active_feedforward_pwm_x10 = 0;
    frame->active_p_term_x10 = 0;
    frame->active_integral_term_x10 = 0;
}

void SpeedCalibrationTest_Run(void)
{
    SpeedCalibrationTestCase test_case = SpeedCalibration_GetSelectedCase();
    uint32_t duration_ms = SpeedCalibration_GetDurationMs(test_case);
    uint32_t total_ms = duration_ms + SPEED_CAL_STOP_HOLD_MS;
    uint32_t start_ms;
    uint32_t elapsed_ms = 0U;
    uint32_t next_vofa_ms = SPEED_CAL_VOFA_PERIOD_MS;
    SpeedCalibrationVofaFrame frame;

    g_speed_cal_control_late_count = 0U;
    g_speed_cal_vofa_tx_count = 0U;
    SpeedPI_Reset();
    Motor_Coast();
    Encoder_ClearCount();
    delay_ms(SPEED_CAL_PREP_HOLD_MS);
    SpeedPI_Reset();
    Motor_Coast();
    Encoder_ClearCount();
    board_clear_control_ticks();
    start_ms = board_millis();

    if (duration_ms == 0U)
    {
        SpeedCalibration_ControlUnsupported(test_case, &frame);
        SpeedCalibration_SendVofaFrame(&frame);
        while (1)
        {
            Motor_Coast();
            delay_ms(100U);
        }
    }

    while (elapsed_ms <= total_ms)
    {
        while (board_consume_control_tick() == 0U)
        {
        }

        if (board_pending_control_ticks() != 0U)
        {
            g_speed_cal_control_late_count++;
        }

        elapsed_ms = board_millis() - start_ms;
        if (elapsed_ms < duration_ms)
        {
            if (SpeedCalibration_IsBothCase(test_case) != 0U)
            {
                SpeedCalibration_ControlBothEqual(elapsed_ms, &frame);
            }
            else if (SpeedCalibration_IsClosedCase(test_case) != 0U)
            {
                SpeedCalibration_ControlClosedLoop(test_case, elapsed_ms, &frame);
            }
            else
            {
                SpeedCalibration_ControlOpenLoop(test_case, elapsed_ms, &frame);
            }
        }
        else
        {
            SpeedCalibration_ControlStop(test_case, elapsed_ms, &frame);
        }

        if (elapsed_ms >= next_vofa_ms)
        {
            frame.control_late_count = (int32_t)g_speed_cal_control_late_count;
            frame.vofa_tx_count = (int32_t)g_speed_cal_vofa_tx_count;
            SpeedCalibration_SendVofaFrame(&frame);
            next_vofa_ms += SPEED_CAL_VOFA_PERIOD_MS;
        }
    }

    SpeedPI_Reset();
    Motor_Coast();

    while (1)
    {
        Motor_Coast();
        delay_ms(100U);
    }
}




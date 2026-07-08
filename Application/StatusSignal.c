#include "Application/StatusSignal.h"

#include "Hardware/Buzzer.h"

typedef enum
{
    SIGNAL_NONE = 0,
    SIGNAL_KEY_POINT = 1,
    SIGNAL_FINISH = 2,
    SIGNAL_ERROR = 3
} SignalPattern;

static SignalPattern pattern;
static uint8_t phase;
static uint16_t remaining_ms;

static uint8_t PhaseCount(SignalPattern value)
{
    if (value == SIGNAL_KEY_POINT) return 1U;
    if (value == SIGNAL_FINISH) return 5U;
    if (value == SIGNAL_ERROR) return 7U;
    return 0U;
}

static uint16_t PhaseDuration(SignalPattern value, uint8_t value_phase)
{
    if (value == SIGNAL_KEY_POINT) return 300U;

    if (value == SIGNAL_FINISH)
    {
        static const uint16_t durations[5] = {200U, 120U, 200U, 120U, 300U};
        return durations[value_phase];
    }

    if (value == SIGNAL_ERROR)
    {
        static const uint16_t durations[7] =
            {100U, 100U, 100U, 100U, 100U, 100U, 500U};
        return durations[value_phase];
    }

    return 0U;
}

static void ApplyPhase(void)
{
    if ((phase & 1U) == 0U)
        Buzzer_On();
    else
        Buzzer_Off();
}

static void StartPattern(SignalPattern requested)
{
    if ((requested == SIGNAL_NONE) || (requested < pattern)) return;

    pattern = requested;
    phase = 0U;
    remaining_ms = PhaseDuration(pattern, phase);
    ApplyPhase();
}

void StatusSignal_Init(void)
{
    pattern = SIGNAL_NONE;
    phase = 0U;
    remaining_ms = 0U;
    Buzzer_Off();
}

void StatusSignal_RequestKeyPoint(void)
{
    StartPattern(SIGNAL_KEY_POINT);
}

void StatusSignal_RequestFinish(void)
{
    StartPattern(SIGNAL_FINISH);
}

void StatusSignal_RequestError(void)
{
    StartPattern(SIGNAL_ERROR);
}

void StatusSignal_Update(uint16_t elapsed_ms)
{
    while ((pattern != SIGNAL_NONE) && (elapsed_ms >= remaining_ms))
    {
        elapsed_ms = (uint16_t)(elapsed_ms - remaining_ms);
        phase++;

        if (phase >= PhaseCount(pattern))
        {
            pattern = SIGNAL_NONE;
            remaining_ms = 0U;
            Buzzer_Off();
        }
        else
        {
            remaining_ms = PhaseDuration(pattern, phase);
            ApplyPhase();
        }
    }

    if (pattern != SIGNAL_NONE)
        remaining_ms = (uint16_t)(remaining_ms - elapsed_ms);
}

uint8_t StatusSignal_IsBusy(void)
{
    return (pattern != SIGNAL_NONE) ? 1U : 0U;
}

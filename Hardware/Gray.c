#include "Gray.h"

#include "ti_msp_dl_config.h"
#include "Public/Board/board.h"

#ifndef GRAY_SAMPLE_ON_RISING_EDGE
#define GRAY_SAMPLE_ON_RISING_EDGE 0
#endif

#ifndef GRAY_FIRST_BIT_IS_BIT0
#define GRAY_FIRST_BIT_IS_BIT0 1
#endif

#ifndef GRAY_BLACK_IS_1
#define GRAY_BLACK_IS_1 0
#endif

#define GRAY_SERIAL_DELAY_US (5U)

static uint8_t g_gray_digital;
static int16_t g_gray_error;
static uint8_t g_gray_lost;

static void Gray_SetClockLow(void)
{
    DL_GPIO_clearPins(GRAY_PORT, GRAY_CLK_PIN);
}

static void Gray_SetClockHigh(void)
{
    DL_GPIO_setPins(GRAY_PORT, GRAY_CLK_PIN);
}

uint8_t Gray_ReadDATRaw(void)
{
    return (DL_GPIO_readPins(GRAY_PORT, GRAY_DAT_PIN) != 0U) ? 1U : 0U;
}

static uint8_t Gray_SerialRead(void)
{
    uint8_t ret = 0U;
    uint8_t i;

    Gray_SetClockLow();

    for (i = 0U; i < 8U; i++)
    {
        uint8_t bit_position;
        uint8_t dat;

#if GRAY_FIRST_BIT_IS_BIT0
        bit_position = i;
#else
        bit_position = (uint8_t)(7U - i);
#endif

        Gray_SetClockHigh();

#if GRAY_SAMPLE_ON_RISING_EDGE
        delay_us(GRAY_SERIAL_DELAY_US);
        dat = Gray_ReadDATRaw();
        Gray_SetClockLow();
#else
        delay_us(GRAY_SERIAL_DELAY_US);
        Gray_SetClockLow();
        dat = Gray_ReadDATRaw();
#endif

        if (dat != 0U)
        {
            ret |= (uint8_t)(1U << bit_position);
        }
    }

    return ret;
}

static uint8_t Gray_CalculateBlackMask(uint8_t digital)
{
#if GRAY_BLACK_IS_1
    return digital;
#else
    return (uint8_t)(~digital);
#endif
}

static int16_t Gray_CalculateError(uint8_t black_mask)
{
    static const int8_t weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};
    int16_t sum = 0;
    uint8_t count = 0U;
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        if ((black_mask & (uint8_t)(1U << i)) != 0U)
        {
            sum += weights[i];
            count++;
        }
    }

    if (count == 0U)
    {
        return 0;
    }

    return (int16_t)(sum / (int16_t)count);
}

static uint8_t Gray_CalculateLost(uint8_t black_mask)
{
    return (black_mask == 0U) ? 1U : 0U;
}

void Gray_Init(void)
{
    g_gray_digital = 0U;
    g_gray_error = 0;
    g_gray_lost = 1U;
    Gray_SetClockLow();
}

void Gray_Update(void)
{
    uint8_t black_mask;

    g_gray_digital = Gray_SerialRead();
    black_mask = Gray_CalculateBlackMask(g_gray_digital);
    g_gray_lost = Gray_CalculateLost(black_mask);
    g_gray_error = Gray_CalculateError(black_mask);
}

uint8_t Gray_GetRaw(void)
{
    return g_gray_digital;
}

uint8_t Gray_GetDigital(void)
{
    return g_gray_digital;
}

uint8_t Gray_GetBlackMask_ActiveLow(void)
{
    return (uint8_t)(~g_gray_digital);
}

uint8_t Gray_GetBlackMask_ActiveHigh(void)
{
    return g_gray_digital;
}

uint8_t Gray_GetBlackMask(void)
{
    return Gray_CalculateBlackMask(g_gray_digital);
}

int16_t Gray_GetError(void)
{
    return g_gray_error;
}

uint8_t Gray_IsLost(void)
{
    return g_gray_lost;
}

void Gray_DebugClockPulse(uint16_t count)
{
    uint16_t i;

    for (i = 0U; i < count; i++)
    {
        Gray_SetClockLow();
        delay_ms(50);
        Gray_SetClockHigh();
        delay_ms(50);
    }

    Gray_SetClockLow();
}

uint8_t Gray_DebugReadDAT8Times(void)
{
    uint8_t i;
    uint8_t value = 0U;

    for (i = 0U; i < 8U; i++)
    {
        if (Gray_ReadDATRaw() != 0U)
        {
            value |= (uint8_t)(1U << i);
        }
        delay_ms(50);
    }

    return value;
}

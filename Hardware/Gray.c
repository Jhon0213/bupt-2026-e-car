#include "Gray.h"

#include "Public/Board/board.h"
#include "ti_msp_dl_config.h"

#ifndef GRAY_BLACK_IS_1
#define GRAY_BLACK_IS_1 1
#endif

#define GRAY_SENSOR_I2C_ADDR      (0x5CU)
#define GRAY_SENSOR_STATE_REG     (0x05U)
#define GRAY_SENSOR_CHANNEL_COUNT (6U)
#define GRAY_SENSOR_CHANNEL_MASK  (0x3FU)
#define GRAY_I2C_DELAY_US         (8)
#define GRAY_I2C_TIMEOUT          (1000U)

#define GRAY_I2C_STAGE_OK              (0U)
#define GRAY_I2C_STAGE_BAD_ARG         (1U)
#define GRAY_I2C_STAGE_SCL_STUCK       (2U)
#define GRAY_I2C_STAGE_START_STUCK     (3U)
#define GRAY_I2C_STAGE_ADDR_W_NACK     (4U)
#define GRAY_I2C_STAGE_REG_NACK        (5U)
#define GRAY_I2C_STAGE_ADDR_R_NACK     (6U)

static uint8_t g_gray_digital;
static int16_t g_gray_error;
static uint8_t g_gray_lost;
static uint8_t g_gray_i2c_ok;
static uint8_t g_gray_i2c_stage;
static uint32_t g_gray_i2c_status;

static void Gray_I2CDelay(void)
{
    delay_us(GRAY_I2C_DELAY_US);
}

static void Gray_I2CInitPins(void)
{
    DL_GPIO_initDigitalInputFeatures(GPIO_LINE_I2C_SCL_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_LINE_I2C_SDA_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearPins(GPIO_LINE_I2C_SCL_PORT, GPIO_LINE_I2C_SCL_PIN);
    DL_GPIO_clearPins(GPIO_LINE_I2C_SDA_PORT, GPIO_LINE_I2C_SDA_PIN);
    DL_GPIO_disableOutput(GPIO_LINE_I2C_SCL_PORT, GPIO_LINE_I2C_SCL_PIN);
    DL_GPIO_disableOutput(GPIO_LINE_I2C_SDA_PORT, GPIO_LINE_I2C_SDA_PIN);
}

static void Gray_SdaLow(void)
{
    DL_GPIO_clearPins(GPIO_LINE_I2C_SDA_PORT, GPIO_LINE_I2C_SDA_PIN);
    DL_GPIO_enableOutput(GPIO_LINE_I2C_SDA_PORT, GPIO_LINE_I2C_SDA_PIN);
}

static void Gray_SdaRelease(void)
{
    DL_GPIO_disableOutput(GPIO_LINE_I2C_SDA_PORT, GPIO_LINE_I2C_SDA_PIN);
}

static void Gray_SclLow(void)
{
    DL_GPIO_clearPins(GPIO_LINE_I2C_SCL_PORT, GPIO_LINE_I2C_SCL_PIN);
    DL_GPIO_enableOutput(GPIO_LINE_I2C_SCL_PORT, GPIO_LINE_I2C_SCL_PIN);
}

static uint8_t Gray_SclRelease(void)
{
    uint32_t timeout = GRAY_I2C_TIMEOUT;

    DL_GPIO_disableOutput(GPIO_LINE_I2C_SCL_PORT, GPIO_LINE_I2C_SCL_PIN);
    while ((DL_GPIO_readPins(GPIO_LINE_I2C_SCL_PORT,
                             GPIO_LINE_I2C_SCL_PIN) &
            GPIO_LINE_I2C_SCL_PIN) == 0U)
    {
        if (timeout == 0U)
        {
            g_gray_i2c_stage = GRAY_I2C_STAGE_SCL_STUCK;
            g_gray_i2c_status = 0x00000002UL;
            return 0U;
        }
        timeout--;
    }

    return 1U;
}

static uint8_t Gray_ReadSda(void)
{
    return ((DL_GPIO_readPins(GPIO_LINE_I2C_SDA_PORT,
                              GPIO_LINE_I2C_SDA_PIN) &
             GPIO_LINE_I2C_SDA_PIN) != 0U) ? 1U : 0U;
}

static uint8_t Gray_I2CStart(void)
{
    Gray_SdaRelease();
    if (Gray_SclRelease() == 0U)
    {
        return 0U;
    }
    Gray_I2CDelay();

    if (Gray_ReadSda() == 0U)
    {
        g_gray_i2c_stage = GRAY_I2C_STAGE_START_STUCK;
        g_gray_i2c_status = 0x00000001UL;
        return 0U;
    }

    Gray_SdaLow();
    Gray_I2CDelay();
    Gray_SclLow();
    Gray_I2CDelay();
    return 1U;
}

static void Gray_I2CStop(void)
{
    Gray_SdaLow();
    Gray_I2CDelay();
    (void)Gray_SclRelease();
    Gray_I2CDelay();
    Gray_SdaRelease();
    Gray_I2CDelay();
}

static uint8_t Gray_I2CWriteByte(uint8_t value)
{
    uint8_t mask;
    uint8_t ack;

    for (mask = 0x80U; mask != 0U; mask >>= 1)
    {
        if ((value & mask) != 0U)
        {
            Gray_SdaRelease();
        }
        else
        {
            Gray_SdaLow();
        }

        Gray_I2CDelay();
        if (Gray_SclRelease() == 0U)
        {
            return 0U;
        }
        Gray_I2CDelay();
        Gray_SclLow();
        Gray_I2CDelay();
    }

    Gray_SdaRelease();
    Gray_I2CDelay();
    if (Gray_SclRelease() == 0U)
    {
        return 0U;
    }
    Gray_I2CDelay();
    ack = (Gray_ReadSda() == 0U) ? 1U : 0U;
    Gray_SclLow();
    Gray_I2CDelay();

    return ack;
}

static uint8_t Gray_I2CReadByte(uint8_t ack)
{
    uint8_t value = 0U;
    uint8_t i;

    Gray_SdaRelease();
    for (i = 0U; i < 8U; i++)
    {
        value <<= 1;
        Gray_I2CDelay();
        if (Gray_SclRelease() == 0U)
        {
            return 0U;
        }
        Gray_I2CDelay();
        if (Gray_ReadSda() != 0U)
        {
            value |= 0x01U;
        }
        Gray_SclLow();
        Gray_I2CDelay();
    }

    if (ack != 0U)
    {
        Gray_SdaLow();
    }
    else
    {
        Gray_SdaRelease();
    }

    Gray_I2CDelay();
    (void)Gray_SclRelease();
    Gray_I2CDelay();
    Gray_SclLow();
    Gray_SdaRelease();
    Gray_I2CDelay();

    return value;
}

static uint8_t Gray_I2CWriteAddress(uint8_t addr, uint8_t read, uint8_t fail_stage)
{
    uint8_t wire_addr = (uint8_t)((addr << 1) | (read & 0x01U));

    if (Gray_I2CWriteByte(wire_addr) == 0U)
    {
        if (g_gray_i2c_stage != GRAY_I2C_STAGE_SCL_STUCK)
        {
            g_gray_i2c_stage = fail_stage;
            g_gray_i2c_status = ((uint32_t)wire_addr << 8) | 0x00000001UL;
        }
        return 0U;
    }

    return 1U;
}

static uint8_t Gray_I2CReadRegister(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t i;

    if ((data == 0) || (length == 0U))
    {
        g_gray_i2c_stage = GRAY_I2C_STAGE_BAD_ARG;
        return 0U;
    }

    if (Gray_I2CStart() == 0U)
    {
        Gray_I2CStop();
        return 0U;
    }
    if (Gray_I2CWriteAddress(GRAY_SENSOR_I2C_ADDR, 0U,
                             GRAY_I2C_STAGE_ADDR_W_NACK) == 0U)
    {
        Gray_I2CStop();
        return 0U;
    }
    if (Gray_I2CWriteByte(reg) == 0U)
    {
        if (g_gray_i2c_stage != GRAY_I2C_STAGE_SCL_STUCK)
        {
            g_gray_i2c_stage = GRAY_I2C_STAGE_REG_NACK;
            g_gray_i2c_status = ((uint32_t)reg << 8) | 0x00000001UL;
        }
        Gray_I2CStop();
        return 0U;
    }
    if (Gray_I2CStart() == 0U)
    {
        Gray_I2CStop();
        return 0U;
    }
    if (Gray_I2CWriteAddress(GRAY_SENSOR_I2C_ADDR, 1U,
                             GRAY_I2C_STAGE_ADDR_R_NACK) == 0U)
    {
        Gray_I2CStop();
        return 0U;
    }

    for (i = 0U; i < length; i++)
    {
        data[i] = Gray_I2CReadByte((i + 1U) < length);
    }

    Gray_I2CStop();
    g_gray_i2c_stage = GRAY_I2C_STAGE_OK;
    g_gray_i2c_status = 0U;
    return 1U;
}

static uint8_t Gray_CalculateBlackMask(uint8_t digital)
{
#if GRAY_BLACK_IS_1
    return (uint8_t)(digital & GRAY_SENSOR_CHANNEL_MASK);
#else
    return (uint8_t)(~digital) & GRAY_SENSOR_CHANNEL_MASK;
#endif
}

static int16_t Gray_CalculateError(uint8_t black_mask)
{
    static const int8_t weights[GRAY_SENSOR_CHANNEL_COUNT] = {-5, -3, -1, 1, 3, 5};
    int16_t sum = 0;
    uint8_t count = 0U;
    uint8_t i;

    black_mask &= GRAY_SENSOR_CHANNEL_MASK;
    for (i = 0U; i < GRAY_SENSOR_CHANNEL_COUNT; i++)
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
    return ((black_mask & GRAY_SENSOR_CHANNEL_MASK) == 0U) ? 1U : 0U;
}

void Gray_Init(void)
{
    Gray_I2CInitPins();
    g_gray_digital = 0U;
    g_gray_error = 0;
    g_gray_lost = 1U;
    g_gray_i2c_ok = 0U;
    g_gray_i2c_stage = GRAY_I2C_STAGE_OK;
    g_gray_i2c_status = 0U;
}

void Gray_Update(void)
{
    uint8_t digital;
    uint8_t black_mask;

    if (Gray_I2CReadRegister(GRAY_SENSOR_STATE_REG, &digital, 1U) == 0U)
    {
        g_gray_digital = 0U;
        g_gray_error = 0;
        g_gray_lost = 1U;
        g_gray_i2c_ok = 0U;
        return;
    }

    g_gray_i2c_ok = 1U;
    g_gray_digital = (uint8_t)(digital & GRAY_SENSOR_CHANNEL_MASK);
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
    return (uint8_t)(~g_gray_digital) & GRAY_SENSOR_CHANNEL_MASK;
}

uint8_t Gray_GetBlackMask_ActiveHigh(void)
{
    return (uint8_t)(g_gray_digital & GRAY_SENSOR_CHANNEL_MASK);
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

uint8_t Gray_IsI2COk(void)
{
    return g_gray_i2c_ok;
}

uint8_t Gray_GetI2CStage(void)
{
    return g_gray_i2c_stage;
}

uint32_t Gray_GetI2CStatus(void)
{
    return g_gray_i2c_status;
}

uint8_t Gray_I2CProbeAddress(uint8_t addr)
{
    if (addr > 0x7FU)
    {
        g_gray_i2c_stage = GRAY_I2C_STAGE_BAD_ARG;
        return 0U;
    }

    if (Gray_I2CStart() == 0U)
    {
        Gray_I2CStop();
        return 0U;
    }

    if (Gray_I2CWriteAddress(addr, 0U, GRAY_I2C_STAGE_ADDR_W_NACK) == 0U)
    {
        Gray_I2CStop();
        return 0U;
    }

    Gray_I2CStop();
    g_gray_i2c_stage = GRAY_I2C_STAGE_OK;
    g_gray_i2c_status = 0U;
    return 1U;
}

uint8_t Gray_ReadDATRaw(void)
{
    return (uint8_t)(g_gray_digital & 0x01U);
}

void Gray_DebugClockPulse(uint16_t count)
{
    uint16_t i;

    Gray_I2CInitPins();
    for (i = 0U; i < count; i++)
    {
        Gray_SclLow();
        Gray_I2CDelay();
        (void)Gray_SclRelease();
        Gray_I2CDelay();
    }
}

uint8_t Gray_DebugReadDAT8Times(void)
{
    Gray_Update();
    return g_gray_digital;
}


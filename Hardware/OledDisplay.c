#include "Hardware/OledDisplay.h"

#include <string.h>

#include "Public/Board/board.h"
#include "ti_msp_dl_config.h"

#define OLED_I2C_ADDR       0x3CU
#define OLED_WIDTH          128U
#define OLED_PAGES          8U
#define OLED_CHARS_PER_LINE 21U

#define OLED_SDA_PORT       GPIOA
#define OLED_SDA_PIN        DL_GPIO_PIN_0
#define OLED_SDA_IOMUX      IOMUX_PINCM1
#define OLED_SCL_PORT       GPIOA
#define OLED_SCL_PIN        DL_GPIO_PIN_1
#define OLED_SCL_IOMUX      IOMUX_PINCM2

static uint8_t g_oled_buffer[OLED_WIDTH * OLED_PAGES];

static void I2CDelay(void)
{
    delay_us(10);
}

static void SdaLow(void)
{
    DL_GPIO_clearPins(OLED_SDA_PORT, OLED_SDA_PIN);
    DL_GPIO_enableOutput(OLED_SDA_PORT, OLED_SDA_PIN);
}

static void SdaRelease(void)
{
    DL_GPIO_disableOutput(OLED_SDA_PORT, OLED_SDA_PIN);
}

static void SclLow(void)
{
    DL_GPIO_clearPins(OLED_SCL_PORT, OLED_SCL_PIN);
    DL_GPIO_enableOutput(OLED_SCL_PORT, OLED_SCL_PIN);
}

static void SclRelease(void)
{
    DL_GPIO_disableOutput(OLED_SCL_PORT, OLED_SCL_PIN);
}

static void I2CStart(void)
{
    SdaRelease();
    SclRelease();
    I2CDelay();
    SdaLow();
    I2CDelay();
    SclLow();
}

static void I2CStop(void)
{
    SdaLow();
    I2CDelay();
    SclRelease();
    I2CDelay();
    SdaRelease();
    I2CDelay();
}

static uint8_t I2CWriteByte(uint8_t value)
{
    uint8_t mask;
    uint8_t ack;

    for (mask = 0x80U; mask != 0U; mask >>= 1)
    {
        if ((value & mask) != 0U)
            SdaRelease();
        else
            SdaLow();

        I2CDelay();
        SclRelease();
        I2CDelay();
        SclLow();
    }

    SdaRelease();
    I2CDelay();
    SclRelease();
    I2CDelay();
    ack = (DL_GPIO_readPins(OLED_SDA_PORT, OLED_SDA_PIN) == 0U) ? 1U : 0U;
    SclLow();
    return ack;
}

static void OledSendCommand(uint8_t command)
{
    I2CStart();
    (void)I2CWriteByte((uint8_t)(OLED_I2C_ADDR << 1));
    (void)I2CWriteByte(0x00U);
    (void)I2CWriteByte(command);
    I2CStop();
}

static void OledSendData(const uint8_t *data, uint8_t count)
{
    uint8_t i;

    I2CStart();
    (void)I2CWriteByte((uint8_t)(OLED_I2C_ADDR << 1));
    (void)I2CWriteByte(0x40U);
    for (i = 0U; i < count; i++)
    {
        (void)I2CWriteByte(data[i]);
    }
    I2CStop();
}

static void GetGlyph(char ch, uint8_t glyph[5])
{
    static const uint8_t font_20_5a[][5] = {
        {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00},
        {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
        {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
        {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
        {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00},
        {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
        {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08},
        {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
        {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
        {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
        {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
        {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E},
        {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
        {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
        {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},
        {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
        {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
        {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F},
        {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
        {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E},
        {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
        {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
        {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
        {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07},
        {0x61,0x51,0x49,0x45,0x43}
    };

    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch = (char)(ch - ('a' - 'A'));
    }

    if ((ch < ' ') || (ch > 'Z'))
    {
        ch = '?';
    }

    memcpy(glyph, font_20_5a[(uint8_t)ch - (uint8_t)' '], 5U);
}

void OledDisplay_Init(void)
{
    DL_GPIO_initDigitalInputFeatures(OLED_SDA_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(OLED_SCL_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_clearPins(OLED_SDA_PORT, OLED_SDA_PIN);
    DL_GPIO_clearPins(OLED_SCL_PORT, OLED_SCL_PIN);
    SdaRelease();
    SclRelease();

    delay_ms(50);

    OledSendCommand(0xAEU);
    OledSendCommand(0x20U);
    OledSendCommand(0x00U);
    OledSendCommand(0xB0U);
    OledSendCommand(0xC8U);
    OledSendCommand(0x00U);
    OledSendCommand(0x10U);
    OledSendCommand(0x40U);
    OledSendCommand(0x81U);
    OledSendCommand(0x7FU);
    OledSendCommand(0xA1U);
    OledSendCommand(0xA6U);
    OledSendCommand(0xA8U);
    OledSendCommand(0x3FU);
    OledSendCommand(0xA4U);
    OledSendCommand(0xD3U);
    OledSendCommand(0x00U);
    OledSendCommand(0xD5U);
    OledSendCommand(0x80U);
    OledSendCommand(0xD9U);
    OledSendCommand(0xF1U);
    OledSendCommand(0xDAU);
    OledSendCommand(0x12U);
    OledSendCommand(0xDBU);
    OledSendCommand(0x40U);
    OledSendCommand(0x8DU);
    OledSendCommand(0x14U);
    OledSendCommand(0xAFU);
    OledSendCommand(0xA5U);
    delay_ms(300);
    OledSendCommand(0xA4U);

    OledDisplay_Clear();
    OledDisplay_Update();
}

void OledDisplay_Clear(void)
{
    memset(g_oled_buffer, 0, sizeof(g_oled_buffer));
}

void OledDisplay_WriteLine(uint8_t line, const char *text)
{
    uint8_t i;
    uint8_t glyph[5];
    uint8_t *row;

    if (line >= OLED_PAGES)
    {
        return;
    }

    row = &g_oled_buffer[(uint16_t)line * OLED_WIDTH];
    memset(row, 0, OLED_WIDTH);

    if (text == 0)
    {
        return;
    }

    for (i = 0U; (i < OLED_CHARS_PER_LINE) && (text[i] != '\0'); i++)
    {
        uint8_t x = (uint8_t)(i * 6U);
        GetGlyph(text[i], glyph);
        row[x + 0U] = glyph[0];
        row[x + 1U] = glyph[1];
        row[x + 2U] = glyph[2];
        row[x + 3U] = glyph[3];
        row[x + 4U] = glyph[4];
        row[x + 5U] = 0x00U;
    }
}

void OledDisplay_Update(void)
{
    uint8_t page;
    uint8_t offset;

    for (page = 0U; page < OLED_PAGES; page++)
    {
        OledSendCommand((uint8_t)(0xB0U + page));
        OledSendCommand(0x00U);
        OledSendCommand(0x10U);

        for (offset = 0U; offset < OLED_WIDTH; offset += 16U)
        {
            OledSendData(&g_oled_buffer[(uint16_t)page * OLED_WIDTH + offset],
                         16U);
        }
    }
}

#include "Hardware/KeyInput.h"

#include "ti_msp_dl_config.h"

#define KEY_PORT             GPIOB
#define KEY_K1_PIN           DL_GPIO_PIN_10
#define KEY_K1_IOMUX         IOMUX_PINCM27
#define KEY_K2_PIN           DL_GPIO_PIN_11
#define KEY_K2_IOMUX         IOMUX_PINCM28
#define KEY_DEBOUNCE_MS      25U

typedef struct
{
    uint32_t pin;
    uint8_t stable_pressed;
    uint8_t raw_pressed;
    uint32_t changed_ms;
} KeyState;

static KeyState g_keys[2];

static uint8_t ReadPressed(uint32_t pin)
{
    return (DL_GPIO_readPins(KEY_PORT, pin) == 0U) ? 1U : 0U;
}

static void InitOneKey(KeyState *key, uint32_t pin)
{
    key->pin = pin;
    key->stable_pressed = ReadPressed(pin);
    key->raw_pressed = key->stable_pressed;
    key->changed_ms = 0U;
}

void KeyInput_Init(void)
{
    DL_GPIO_initDigitalInputFeatures(KEY_K1_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_ENABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(KEY_K2_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_ENABLE,
                                     DL_GPIO_WAKEUP_DISABLE);

    InitOneKey(&g_keys[0], KEY_K1_PIN);
    InitOneKey(&g_keys[1], KEY_K2_PIN);
}

uint8_t KeyInput_Update(uint32_t now_ms)
{
    uint8_t i;
    uint8_t events = 0U;

    for (i = 0U; i < 2U; i++)
    {
        uint8_t raw = ReadPressed(g_keys[i].pin);

        if (raw != g_keys[i].raw_pressed)
        {
            g_keys[i].raw_pressed = raw;
            g_keys[i].changed_ms = now_ms;
        }
        else if ((raw != g_keys[i].stable_pressed) &&
                 ((now_ms - g_keys[i].changed_ms) >= KEY_DEBOUNCE_MS))
        {
            g_keys[i].stable_pressed = raw;
            if (raw != 0U)
            {
                events |= (i == 0U) ? KEY_INPUT_K1_PRESSED :
                                      KEY_INPUT_K2_PRESSED;
            }
        }
    }

    return events;
}

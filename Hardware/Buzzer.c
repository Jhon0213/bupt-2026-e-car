#include "Hardware/Buzzer.h"

#include "ti_msp_dl_config.h"

#define BUZZER_PORT   GPIOA
#define BUZZER_PIN    DL_GPIO_PIN_13
#define BUZZER_IOMUX  IOMUX_PINCM35

static uint8_t buzzer_on;

void Buzzer_Init(void)
{
    DL_GPIO_initDigitalOutput(BUZZER_IOMUX);
    DL_GPIO_clearPins(BUZZER_PORT, BUZZER_PIN);
    DL_GPIO_enableOutput(BUZZER_PORT, BUZZER_PIN);
    buzzer_on = 0U;
}

void Buzzer_On(void)
{
    DL_GPIO_setPins(BUZZER_PORT, BUZZER_PIN);
    buzzer_on = 1U;
}

void Buzzer_Off(void)
{
    DL_GPIO_clearPins(BUZZER_PORT, BUZZER_PIN);
    buzzer_on = 0U;
}

uint8_t Buzzer_IsOn(void)
{
    return buzzer_on;
}

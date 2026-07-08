#include "Hardware/LaserRelay.h"

#include "ti_msp_dl_config.h"

#define LASER_RELAY_PORT   GPIOA
#define LASER_RELAY_PIN    DL_GPIO_PIN_12
#define LASER_RELAY_IOMUX  IOMUX_PINCM34

static uint8_t relay_on;

void LaserRelay_Init(void)
{
    DL_GPIO_initDigitalOutput(LASER_RELAY_IOMUX);
    DL_GPIO_clearPins(LASER_RELAY_PORT, LASER_RELAY_PIN);
    DL_GPIO_enableOutput(LASER_RELAY_PORT, LASER_RELAY_PIN);
    relay_on = 0U;
}

void LaserRelay_On(void)
{
    DL_GPIO_setPins(LASER_RELAY_PORT, LASER_RELAY_PIN);
    relay_on = 1U;
}

void LaserRelay_Off(void)
{
    DL_GPIO_clearPins(LASER_RELAY_PORT, LASER_RELAY_PIN);
    relay_on = 0U;
}

uint8_t LaserRelay_IsOn(void)
{
    return relay_on;
}

#include "Hardware/TrackZone.h"

#include "ti_msp_dl_config.h"

/* 3.3 V push-pull outputs: PB17 = ZONE0, PB18 = ZONE1. */
#define TRACK_ZONE_PORT       GPIOB
#define TRACK_ZONE0_PIN       DL_GPIO_PIN_17
#define TRACK_ZONE0_IOMUX     IOMUX_PINCM43
#define TRACK_ZONE1_PIN       DL_GPIO_PIN_18
#define TRACK_ZONE1_IOMUX     IOMUX_PINCM44
#define TRACK_ZONE_PIN_MASK   (TRACK_ZONE0_PIN | TRACK_ZONE1_PIN)

static uint8_t g_track_zone;

void TrackZone_Init(void)
{
    DL_GPIO_initDigitalOutput(TRACK_ZONE0_IOMUX);
    DL_GPIO_initDigitalOutput(TRACK_ZONE1_IOMUX);

    /* Establish the required AB code before enabling the output drivers. */
    DL_GPIO_clearPins(TRACK_ZONE_PORT, TRACK_ZONE_PIN_MASK);
    DL_GPIO_enableOutput(TRACK_ZONE_PORT, TRACK_ZONE_PIN_MASK);
    g_track_zone = TRACK_ZONE_AB;
}

void TrackZone_Set(uint8_t zone)
{
    uint32_t value = 0U;

    zone &= 0x03U;
    if ((zone & 0x01U) != 0U) value |= TRACK_ZONE0_PIN;
    if ((zone & 0x02U) != 0U) value |= TRACK_ZONE1_PIN;

    DL_GPIO_writePinsVal(TRACK_ZONE_PORT, TRACK_ZONE_PIN_MASK, value);
    g_track_zone = zone;
}

uint8_t TrackZone_Get(void)
{
    return g_track_zone;
}

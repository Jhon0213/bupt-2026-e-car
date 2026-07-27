#ifndef HARDWARE_TRACK_ZONE_H_
#define HARDWARE_TRACK_ZONE_H_

#include <stdint.h>

#define TRACK_ZONE_AB 0U
#define TRACK_ZONE_BC 1U
#define TRACK_ZONE_CD 2U
#define TRACK_ZONE_DA 3U

void TrackZone_Init(void);
void TrackZone_Set(uint8_t zone);
uint8_t TrackZone_Get(void);

#endif

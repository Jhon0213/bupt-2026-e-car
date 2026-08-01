#ifndef COMMUNICATION_VEHICLE_STATE_PUBLISHER_H_
#define COMMUNICATION_VEHICLE_STATE_PUBLISHER_H_

#include <stdint.h>

#define INTERBOARD_PUBLISH_PERIOD_MS 20U

void VehicleStatePublisher_Init(uint32_t now_ms);
void VehicleStatePublisher_Process(uint32_t now_ms);

#endif
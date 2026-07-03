#ifndef HARDWARE_IMU_H
#define HARDWARE_IMU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float gyro_z_dps;
    float yaw_deg;
    uint32_t gyro_frame_count;
    uint32_t yaw_frame_count;
    uint32_t checksum_error_count;
    uint32_t rx_byte_count;
} IMU_Data;

/* UART1: PA17 = TX, PA18 = RX, 9600 baud. */
void IMU_Init(void);
void IMU_GetData(IMU_Data *data);
bool IMU_IsReceiving(void);
bool IMU_SetHostBaudRate(uint32_t baud_rate);

/* Compatibility accessors used by the existing control module. */
float GyroZ(void);
float Yaw(void);

/* Configuration commands described by the module data sheet. */
void IMU_ZeroYaw(void);
void IMU_StartBiasCalibration(void);
void IMU_SaveConfiguration(void);

#endif

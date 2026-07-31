#ifndef HARDWARE_IMU_H
#define HARDWARE_IMU_H

#include <stdbool.h>
#include <stdint.h>

#define IMU_MODULE_LEGACY 1U
#define IMU_MODULE_AXIS6  2U

#define IMU_SELECTED_MODULE IMU_MODULE_AXIS6

typedef struct
{
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t roll_raw;
    int16_t pitch_raw;
    int16_t yaw_raw;
    int16_t quat_q0_raw;
    int16_t quat_q1_raw;
    int16_t quat_q2_raw;
    int16_t quat_q3_raw;
    uint32_t gyro_frame_count;
    uint32_t accel_frame_count;
    uint32_t yaw_frame_count;
    uint32_t quaternion_frame_count;
    uint32_t checksum_error_count;
    uint32_t rx_byte_count;
} IMU_Data;

/* UART1: PB4 = TX, PB5 = RX; baud rate follows IMU_SELECTED_MODULE. */
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
void IMU_SetOutputRate100Hz(void);

#endif
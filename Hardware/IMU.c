#include "IMU.h"

#include "ti_msp_dl_config.h"

#if IMU_SELECTED_MODULE == IMU_MODULE_LEGACY
#define IMU_FRAME_HEADER          0x5AU
#define IMU_FRAME_GYRO_Z          0xAAU
#define IMU_FRAME_YAW             0xBBU
#define IMU_FRAME_SIZE            5U
#elif IMU_SELECTED_MODULE == IMU_MODULE_AXIS6
#define IMU_FRAME_HEADER          0x5AU
#define IMU_FRAME_GYRO_Z          0xAAU
#define IMU_FRAME_YAW             0xBBU
#define IMU_FRAME_ACCEL           0xCCU
#define IMU_FRAME_QUATERNION      0xDDU
#define IMU_FRAME_REGISTER        0xEEU
#define IMU_FRAME_SIZE            11U
#else
#error "Unsupported IMU_SELECTED_MODULE"
#endif

static volatile IMU_Data g_imu_data;
static uint8_t g_rx_frame[IMU_FRAME_SIZE];
static uint8_t g_rx_index;

static const uint8_t g_cmd_unlock[5] = {0x55U, 0xAAU, 0x13U, 0x8EU, 0x5FU};
#if IMU_SELECTED_MODULE == IMU_MODULE_LEGACY
static const uint8_t g_cmd_yaw_zero[5] = {0x55U, 0xAAU, 0x15U, 0x00U, 0x00U};
static const uint8_t g_cmd_rate_100hz[5] = {0x55U, 0xAAU, 0x02U, 0x09U, 0x00U};
#else
static const uint8_t g_cmd_yaw_zero[5] = {0x55U, 0xAAU, 0x0AU, 0x04U, 0x00U};
#endif
static const uint8_t g_cmd_bias_calibration[5] = {0x55U, 0xAAU, 0x0AU, 0x01U, 0x00U};
static const uint8_t g_cmd_save[5] = {0x55U, 0xAAU, 0x00U, 0x00U, 0x00U};

static void IMU_SendBytes(const uint8_t *data, uint32_t length)
{
    uint32_t i;

    for (i = 0U; i < length; i++)
    {
        while (DL_UART_isBusy(UART_1_INST))
        {
        }
        DL_UART_Main_transmitData(UART_1_INST, data[i]);
    }
}

static void IMU_DelayMs(uint32_t milliseconds)
{
    while (milliseconds > 0U)
    {
        delay_cycles(CPUCLK_FREQ / 1000U);
        milliseconds--;
    }
}

#if IMU_SELECTED_MODULE == IMU_MODULE_LEGACY
static void IMU_ParseByte(uint8_t byte)
{
    uint8_t checksum;
    int16_t raw;

    if (g_rx_index == 0U)
    {
        if (byte == IMU_FRAME_HEADER)
        {
            g_rx_frame[g_rx_index++] = byte;
        }
        return;
    }

    if (g_rx_index == 1U)
    {
        if ((byte == IMU_FRAME_GYRO_Z) || (byte == IMU_FRAME_YAW))
        {
            g_rx_frame[g_rx_index++] = byte;
        }
        else
        {
            g_rx_index = (byte == IMU_FRAME_HEADER) ? 1U : 0U;
        }
        return;
    }

    g_rx_frame[g_rx_index++] = byte;
    if (g_rx_index < IMU_FRAME_SIZE)
    {
        return;
    }

    checksum = (uint8_t)(g_rx_frame[0] + g_rx_frame[1] +
                         g_rx_frame[2] + g_rx_frame[3]);
    if (checksum == g_rx_frame[4])
    {
        raw = (int16_t)(((uint16_t)g_rx_frame[3] << 8) | g_rx_frame[2]);
        if (g_rx_frame[1] == IMU_FRAME_GYRO_Z)
        {
            g_imu_data.gyro_z_raw = raw;
            g_imu_data.gyro_frame_count++;
        }
        else
        {
            g_imu_data.yaw_raw = raw;
            g_imu_data.yaw_frame_count++;
        }
    }
    else
    {
        g_imu_data.checksum_error_count++;
    }
    g_rx_index = 0U;
}
#else
static int16_t IMU_ReadS16(uint32_t low_index)
{
    return (int16_t)(((uint16_t)g_rx_frame[low_index + 1U] << 8) |
                     g_rx_frame[low_index]);
}

static bool IMU_IsAxis6FrameType(uint8_t type)
{
    return (type == IMU_FRAME_GYRO_Z) ||
           (type == IMU_FRAME_YAW) ||
           (type == IMU_FRAME_ACCEL) ||
           (type == IMU_FRAME_QUATERNION) ||
           (type == IMU_FRAME_REGISTER);
}

static void IMU_ParseByte(uint8_t byte)
{
    uint8_t checksum = 0U;
    uint32_t i;

    if (g_rx_index == 0U)
    {
        if (byte == IMU_FRAME_HEADER)
        {
            g_rx_frame[g_rx_index++] = byte;
        }
        return;
    }

    if (g_rx_index == 1U)
    {
        if (!IMU_IsAxis6FrameType(byte))
        {
            g_rx_index = (byte == IMU_FRAME_HEADER) ? 1U : 0U;
            return;
        }
    }

    g_rx_frame[g_rx_index++] = byte;
    if (g_rx_index < IMU_FRAME_SIZE)
    {
        return;
    }

    for (i = 0U; i < (IMU_FRAME_SIZE - 1U); i++)
    {
        checksum = (uint8_t)(checksum + g_rx_frame[i]);
    }

    if (checksum == g_rx_frame[IMU_FRAME_SIZE - 1U])
    {
        switch (g_rx_frame[1])
        {
            case IMU_FRAME_GYRO_Z:
                g_imu_data.gyro_x_raw = IMU_ReadS16(2U);
                g_imu_data.gyro_y_raw = IMU_ReadS16(4U);
                g_imu_data.gyro_z_raw = IMU_ReadS16(6U);
                g_imu_data.gyro_frame_count++;
                break;

            case IMU_FRAME_YAW:
                g_imu_data.roll_raw = IMU_ReadS16(2U);
                g_imu_data.pitch_raw = IMU_ReadS16(4U);
                g_imu_data.yaw_raw = IMU_ReadS16(6U);
                g_imu_data.yaw_frame_count++;
                break;

            case IMU_FRAME_ACCEL:
                g_imu_data.accel_x_raw = IMU_ReadS16(2U);
                g_imu_data.accel_y_raw = IMU_ReadS16(4U);
                g_imu_data.accel_z_raw = IMU_ReadS16(6U);
                g_imu_data.accel_frame_count++;
                break;

            case IMU_FRAME_QUATERNION:
                g_imu_data.quat_q0_raw = IMU_ReadS16(2U);
                g_imu_data.quat_q1_raw = IMU_ReadS16(4U);
                g_imu_data.quat_q2_raw = IMU_ReadS16(6U);
                g_imu_data.quat_q3_raw = IMU_ReadS16(8U);
                g_imu_data.quaternion_frame_count++;
                break;

            default:
                break;
        }
    }
    else
    {
        g_imu_data.checksum_error_count++;
    }
    g_rx_index = 0U;
}
#endif

void IMU_Init(void)
{
    g_imu_data = (IMU_Data){0};
    g_rx_index = 0U;

#if IMU_SELECTED_MODULE == IMU_MODULE_AXIS6
    (void)IMU_SetHostBaudRate(115200U);
#else
    (void)IMU_SetHostBaudRate(9600U);
#endif

    DL_UART_Main_enableInterrupt(UART_1_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
}

void IMU_GetData(IMU_Data *data)
{
    uint32_t primask;

    if (data == 0)
    {
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    *data = g_imu_data;
    if (primask == 0U)
    {
        __enable_irq();
    }
}

bool IMU_IsReceiving(void)
{
    return (g_imu_data.gyro_frame_count != 0U) ||
           (g_imu_data.yaw_frame_count != 0U);
}

bool IMU_SetHostBaudRate(uint32_t baud_rate)
{
    uint32_t integer_divisor;
    uint32_t fractional_divisor;

    if (baud_rate == 9600U)
    {
        integer_divisor = 26U;
        fractional_divisor = 3U;
    }
    else if (baud_rate == 115200U)
    {
        integer_divisor = 2U;
        fractional_divisor = 11U;
    }
    else
    {
        return false;
    }

    NVIC_DisableIRQ(UART_1_INST_INT_IRQN);
    DL_UART_Main_disable(UART_1_INST);
    DL_UART_Main_setOversampling(UART_1_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_1_INST,
                                   integer_divisor,
                                   fractional_divisor);
    DL_UART_Main_enable(UART_1_INST);
    DL_UART_Main_enableInterrupt(UART_1_INST, DL_UART_MAIN_INTERRUPT_RX);
    g_rx_index = 0U;
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
    return true;
}

float GyroZ(void)
{
    return ((float)g_imu_data.gyro_z_raw * 2000.0f) / 32768.0f;
}

float Yaw(void)
{
    return ((float)g_imu_data.yaw_raw * 180.0f) / 32768.0f;
}

void IMU_ZeroYaw(void)
{
    IMU_SendBytes(g_cmd_unlock, sizeof(g_cmd_unlock));
    IMU_DelayMs(100U);
    IMU_SendBytes(g_cmd_yaw_zero, sizeof(g_cmd_yaw_zero));
    IMU_DelayMs(100U);
    IMU_SendBytes(g_cmd_save, sizeof(g_cmd_save));
}

void IMU_StartBiasCalibration(void)
{
    IMU_SendBytes(g_cmd_unlock, sizeof(g_cmd_unlock));
    IMU_DelayMs(100U);
    IMU_SendBytes(g_cmd_bias_calibration, sizeof(g_cmd_bias_calibration));
}

void IMU_SaveConfiguration(void)
{
    IMU_SendBytes(g_cmd_save, sizeof(g_cmd_save));
}

void IMU_SetOutputRate100Hz(void)
{
#if IMU_SELECTED_MODULE == IMU_MODULE_LEGACY
    IMU_SendBytes(g_cmd_unlock, sizeof(g_cmd_unlock));
    IMU_DelayMs(100U);
    IMU_SendBytes(g_cmd_rate_100hz, sizeof(g_cmd_rate_100hz));
    IMU_DelayMs(100U);
    IMU_SendBytes(g_cmd_save, sizeof(g_cmd_save));
#else
    (void)0;
#endif
}

void UART_1_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART_1_INST))
    {
        case DL_UART_IIDX_RX:
            g_imu_data.rx_byte_count++;
            IMU_ParseByte((uint8_t)DL_UART_Main_receiveData(UART_1_INST));
            break;

        default:
            break;
    }
}

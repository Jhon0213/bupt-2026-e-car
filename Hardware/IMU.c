#include "IMU.h"

#include "ti_msp_dl_config.h"

#define IMU_FRAME_HEADER          (0x5AU)
#define IMU_FRAME_GYRO_Z          (0xAAU)
#define IMU_FRAME_YAW             (0xBBU)
#define IMU_FRAME_SIZE            (5U)

static volatile IMU_Data g_imu_data;
static uint8_t g_rx_frame[IMU_FRAME_SIZE];
static uint8_t g_rx_index;

static const uint8_t g_cmd_unlock[5] = {0x55U, 0xAAU, 0x13U, 0x8EU, 0x5FU};
static const uint8_t g_cmd_yaw_zero[5] = {0x55U, 0xAAU, 0x15U, 0x00U, 0x00U};
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

static void IMU_ParseByte(uint8_t byte)
{
    uint8_t checksum;
    int16_t raw;

    if (g_rx_index == 0U)
    {
        if (byte == IMU_FRAME_HEADER)
        {
            g_rx_frame[0] = byte;
            g_rx_index = 1U;
        }
        return;
    }

    if (g_rx_index == 1U)
    {
        if ((byte == IMU_FRAME_GYRO_Z) || (byte == IMU_FRAME_YAW))
        {
            g_rx_frame[1] = byte;
            g_rx_index = 2U;
        }
        else if (byte == IMU_FRAME_HEADER)
        {
            g_rx_frame[0] = byte;
        }
        else
        {
            g_rx_index = 0U;
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
        raw = (int16_t)(((uint16_t)g_rx_frame[3] << 8) |
                        (uint16_t)g_rx_frame[2]);

        if (g_rx_frame[1] == IMU_FRAME_GYRO_Z)
        {
            g_imu_data.gyro_z_dps = ((float)raw * 2000.0f) / 32768.0f;
            g_imu_data.gyro_frame_count++;
        }
        else
        {
            g_imu_data.yaw_deg = ((float)raw * 180.0f) / 32768.0f;
            g_imu_data.yaw_frame_count++;
        }
    }
    else
    {
        g_imu_data.checksum_error_count++;
    }

    g_rx_index = 0U;
}

void IMU_Init(void)
{
    g_imu_data.gyro_z_dps = 0.0f;
    g_imu_data.yaw_deg = 0.0f;
    g_imu_data.gyro_frame_count = 0U;
    g_imu_data.yaw_frame_count = 0U;
    g_imu_data.checksum_error_count = 0U;
    g_imu_data.rx_byte_count = 0U;
    g_rx_index = 0U;

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
    data->gyro_z_dps = g_imu_data.gyro_z_dps;
    data->yaw_deg = g_imu_data.yaw_deg;
    data->gyro_frame_count = g_imu_data.gyro_frame_count;
    data->yaw_frame_count = g_imu_data.yaw_frame_count;
    data->checksum_error_count = g_imu_data.checksum_error_count;
    data->rx_byte_count = g_imu_data.rx_byte_count;
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
    g_rx_index = 0U;
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);

    return true;
}

float GyroZ(void)
{
    return g_imu_data.gyro_z_dps;
}

float Yaw(void)
{
    return g_imu_data.yaw_deg;
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

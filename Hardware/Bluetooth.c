#include "Bluetooth.h"

#include "ti_msp_dl_config.h"

#define BLUETOOTH_RX_BUFFER_SIZE (128U)
#define BLUETOOTH_RX_BUFFER_MASK (BLUETOOTH_RX_BUFFER_SIZE - 1U)

static volatile uint8_t g_rx_buffer[BLUETOOTH_RX_BUFFER_SIZE];
static volatile uint16_t g_rx_head;
static volatile uint16_t g_rx_tail;
static volatile uint32_t g_received_count;
static volatile uint32_t g_overflow_count;

void Bluetooth_Init(void)
{
    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_received_count = 0U;
    g_overflow_count = 0U;

    DL_UART_Main_enableInterrupt(UART_2_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
}

bool Bluetooth_ReadByte(uint8_t *byte)
{
    uint16_t tail;

    if ((byte == 0) || (g_rx_head == g_rx_tail))
    {
        return false;
    }

    tail = g_rx_tail;
    *byte = g_rx_buffer[tail];
    g_rx_tail = (uint16_t)((tail + 1U) & BLUETOOTH_RX_BUFFER_MASK);
    return true;
}

void Bluetooth_SendByte(uint8_t byte)
{
    while (DL_UART_isBusy(UART_2_INST))
    {
    }
    DL_UART_Main_transmitData(UART_2_INST, byte);
}

void Bluetooth_SendString(const char *text)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        Bluetooth_SendByte((uint8_t)*text++);
    }
}

uint32_t Bluetooth_GetReceivedCount(void)
{
    return g_received_count;
}

uint32_t Bluetooth_GetOverflowCount(void)
{
    return g_overflow_count;
}

void UART_2_INST_IRQHandler(void)
{
    uint8_t byte;
    uint16_t next_head;

    switch (DL_UART_getPendingInterrupt(UART_2_INST))
    {
        case DL_UART_IIDX_RX:
            byte = (uint8_t)DL_UART_Main_receiveData(UART_2_INST);
            g_received_count++;
            next_head = (uint16_t)((g_rx_head + 1U) &
                                   BLUETOOTH_RX_BUFFER_MASK);

            if (next_head == g_rx_tail)
            {
                g_overflow_count++;
            }
            else
            {
                g_rx_buffer[g_rx_head] = byte;
                g_rx_head = next_head;
            }
            break;

        default:
            break;
    }
}

#include "ti_msp_dl_config.h"

#include "Hardware/Bluetooth.h"
#include "Public/Board/board.h"

#include <stdint.h>
#include <stdio.h>

static void PrintReceivedByte(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n') ||
        ((byte >= 0x20U) && (byte <= 0x7EU)))
    {
        printf("%c", (char)byte);
    }
    else
    {
        printf("<%02X>", (unsigned int)byte);
    }
}

int main(void)
{
    uint8_t byte;

    board_init();
    Bluetooth_Init();

    printf("\r\nHC-06D Bluetooth test start\r\n");
    printf("UART2: PA21(TX), PA22(RX), 9600 baud, 8-N-1\r\n");
    printf("Send text from the phone. Received data is echoed back.\r\n\r\n");

    Bluetooth_SendString("MSPM0 Bluetooth ready\r\n");

    while (1)
    {
        while (Bluetooth_ReadByte(&byte))
        {
            PrintReceivedByte(byte);
            Bluetooth_SendByte(byte);
        }
    }
}

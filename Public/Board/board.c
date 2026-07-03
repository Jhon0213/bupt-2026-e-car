#include "board.h"

#include <stdio.h>

#include "ti/driverlib/m0p/dl_core.h"

void board_init(void)
{
    SYSCFG_DL_init();

    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(TIMER_1_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
}

void uart0_send_char(char ch)
{
    while (DL_UART_isBusy(UART_0_INST))
    {
    }
    DL_UART_Main_transmitData(UART_0_INST, (uint8_t)ch);
}

void uart0_send_string(char *str)
{
    if (str == 0)
    {
        return;
    }

    while (*str != '\0')
    {
        uart0_send_char(*str++);
    }
}

#if !defined(__MICROLIB)
#if (__ARMCLIB_VERSION <= 6000000)
struct __FILE
{
    int handle;
};
#endif

FILE __stdout;

void _sys_exit(int x)
{
    (void)x;
}
#endif

int fputc(int ch, FILE *stream)
{
    (void)stream;
    uart0_send_char((char)ch);
    return ch;
}

void delay_us(int microseconds)
{
    delay_cycles((CPUCLK_FREQ / 1000000U) * (uint32_t)microseconds);
}

void delay_ms(int milliseconds)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * (uint32_t)milliseconds);
}

void delay_1us(int microseconds)
{
    delay_us(microseconds);
}

void delay_1ms(int milliseconds)
{
    delay_ms(milliseconds);
}

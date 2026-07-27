#include "board.h"

#include <stdio.h>

#include "ti/driverlib/m0p/dl_core.h"

static volatile uint32_t g_board_millis;
static volatile uint32_t g_control_ticks;

static uint32_t EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void ExitCritical(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

void board_init(void)
{
    SYSCFG_DL_init();
    g_board_millis = 0U;
    g_control_ticks = 0U;

    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
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

uint32_t board_millis(void)
{
    uint32_t primask;
    uint32_t now;

    primask = EnterCritical();
    now = g_board_millis;
    ExitCritical(primask);

    return now;
}

void board_time_advance_ms(uint32_t elapsed_ms)
{
    g_board_millis += elapsed_ms;
}

void board_control_tick_notify(void)
{
    if (g_control_ticks != 0xFFFFFFFFU)
    {
        g_control_ticks++;
    }
}

uint8_t board_consume_control_tick(void)
{
    uint32_t primask;
    uint8_t consumed = 0U;

    primask = EnterCritical();
    if (g_control_ticks != 0U)
    {
        g_control_ticks--;
        consumed = 1U;
    }
    ExitCritical(primask);

    return consumed;
}

uint32_t board_pending_control_ticks(void)
{
    uint32_t primask;
    uint32_t ticks;

    primask = EnterCritical();
    ticks = g_control_ticks;
    ExitCritical(primask);

    return ticks;
}

void board_clear_control_ticks(void)
{
    uint32_t primask;

    primask = EnterCritical();
    g_control_ticks = 0U;
    ExitCritical(primask);
}

void delay_1us(int microseconds)
{
    delay_us(microseconds);
}

void delay_1ms(int milliseconds)
{
    delay_ms(milliseconds);
}

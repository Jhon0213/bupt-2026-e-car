#ifndef UART_H
#define UART_H

#include "ti_msp_dl_config.h"
#include "stdio.h"
void uart0_send_char(char ch);
void uart0_send_string(char* str);

#endif // UART_H
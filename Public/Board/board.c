#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "board.h"
#include "ti/driverlib/m0p/dl_core.h"

volatile unsigned int delay_times = 0;
volatile unsigned char uart_data = 0;

struct SAngle stcAngle;
struct SGyro stcGyro;

void board_init(void)
{
	// SYSCFG初始化
	SYSCFG_DL_init();
	//清除串口中断标志
	NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
	//使能串口中断
	NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
	
//	//清除串口中断标志
//	NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
//	//使能串口中断
//	NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
	
	//清除定时器中断标志
  NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
  //使能定时器中断
  NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
	
	//清除定时器中断标志
  NVIC_ClearPendingIRQ(TIMER_1_INST_INT_IRQN);
  //使能定时器中断
  NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
	
}

//串口发送单个字符
void uart0_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_0_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(UART_0_INST, ch);
}
//串口发送字符串
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart0_send_char(*str++);
    }
}

//串口发送单个字符
void uart1_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_1_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(UART_1_INST, ch);
}
//串口发送字符串
void uart1_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart1_send_char(*str++);
    }
}


void uart0_send_SendByte(uint8_t* data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        uart0_send_char(data[i]);  // 直接发送原始字节
    }
}

void uart1_send_SendByte(uint8_t* data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        uart1_send_char(data[i]);  // 直接发送原始字节
    }
}

#if !defined(__MICROLIB)
//不使用微库的话就需要添加下面的函数
#if (__ARMCLIB_VERSION <= 6000000)
//如果编译器是AC5  就定义下面这个结构体
struct __FILE
{
	int handle;
};
#endif

FILE __stdout;

//定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
	x = x;
}
#endif



/******************************************************************************
 * 数据解析函数：接收0x5A开头的5字节数据帧
******************************************************************************/
void CopeSerial2Data(unsigned char ucData)
{
    static unsigned char ucRxBuffer[11];
    static unsigned char ucRxCnt = 0;

    ucRxBuffer[ucRxCnt++] = ucData;

    if (ucRxBuffer[0] != 0x5A)
    {
        ucRxCnt = 0;
        return;
    }

    if (ucRxCnt < 5) return;

    unsigned char sum = 0;
    if (ucRxBuffer[1] == 0xAA)
    {
        // 角速度帧校验和：0x5A + 0xAA + AzL + AzH 
        sum = ucRxBuffer[0] + ucRxBuffer[1] +
              ucRxBuffer[2] + ucRxBuffer[3] ;

        if (sum != ucRxBuffer[4])
        {
            ucRxCnt = 0;
            return;
        }

        short wz    = (short)((ucRxBuffer[3] << 8) | ucRxBuffer[2]);

        stcGyro.wz    = (float)wz    / 32768.0f * 2000.0f;
    }
    else if (ucRxBuffer[1] == 0xBB)
    {
        // 角度帧校验和：0x5A + 0xBB + YawH + YawL 
        sum = ucRxBuffer[0] + ucRxBuffer[1] +
              ucRxBuffer[2] + ucRxBuffer[3];

        if (sum != ucRxBuffer[4])
        {
            ucRxCnt = 0;
            return;
        }

        short rawYaw = (short)((ucRxBuffer[3] << 8) | ucRxBuffer[2]);
        stcAngle.Yaw = (float)rawYaw / 32768.0f * 180.0f;
    }
    ucRxCnt = 0;
}


//串口的中断服务函数
void UART_0_INST_IRQHandler(void)
{
    //如果产生了串口中断
    switch( DL_UART_getPendingInterrupt(UART_0_INST) )
    {
        case DL_UART_IIDX_RX://如果是接收中断
						// 接收发送过来的数据
            uart_data = DL_UART_Main_receiveData(UART_0_INST);
            // 调用数据解析函数
            CopeSerial2Data(uart_data);
            break;

        default://其他的串口中断
            break;
    }
}

//串口的中断服务函数
void UART_1_INST_IRQHandler(void)
{
    //如果产生了串口中断
    switch( DL_UART_getPendingInterrupt(UART_1_INST) )
    {
        case DL_UART_IIDX_RX://如果是接收中断
            //接发送过来的数据保存在变量中
            uart_data = DL_UART_Main_receiveData(UART_1_INST);
            //将保存的数据再发送出去
            uart1_send_char(uart_data);
            break;

        default://其他的串口中断
            break;
    }
}


/******************************************************************************
 * 返回Z轴角速度（单位：°/s）
******************************************************************************/
float GyroZ(void)
{
    return stcGyro.wz;
}

/******************************************************************************
 * 返回当前Yaw角（Z轴角度），单位°，范围 -180 ~ 180
******************************************************************************/
float Yaw(void)
{
   return stcAngle.Yaw;
}


//解锁指令
uint8_t Key[5] = {0x55, 0xAA, 0x13, 0x8E, 0x5F};
//Z轴角度归零指令
uint8_t Yaw_Zero[5] = {0x55, 0xAA, 0x15, 0x00, 0x00};
//保存指令
uint8_t Save[5] = {0x55, 0xAA, 0x00, 0x00, 0x00};
//获取零偏指令
uint8_t BIAS_CAL[5] = {0x55, 0xAA, 0x0A, 0x01, 0x00};

/****************************************************************************** 
 * 发送 Z轴角度归零命令
******************************************************************************/ 
void sendCaliYawCommand(void) 
{ 
   uart0_send_SendByte(Key, 5);
	 delay_ms(100);
	 uart0_send_SendByte(Yaw_Zero, 5);
	 delay_ms(100);
	 uart0_send_SendByte(Save, 5);
}


/****************************************************************************** 
* 发送校准指令(校准过程中请勿移动，否则会校准失败或者校准效果不好)
******************************************************************************/ 
void performCaliBias(void) 
{ 
   uart0_send_SendByte(Key, 5);
	 delay_ms(100);
	 uart0_send_SendByte(BIAS_CAL, 5);
	 delay_ms(21000);
	 uart0_send_SendByte(Save, 5);
}



//printf函数重定义
int fputc(int ch, FILE *stream)
{
	//当串口0忙的时候等待，不忙的时候再发送传进来的字符
	while( DL_UART_isBusy(UART_1_INST) == true );
	
	DL_UART_Main_transmitData(UART_1_INST, ch);
	
	return ch;
}






/* ================ 延时函数封装 =================== */

void delay_us(int __us) { delay_cycles( (CPUCLK_FREQ / 1000 / 1000)*__us); }
void delay_ms(int __ms) { delay_cycles( (CPUCLK_FREQ / 1000)*__ms); }

void delay_1us(int __us) { delay_cycles( (CPUCLK_FREQ / 1000 / 1000)*__us); }
void delay_1ms(int __ms) { delay_cycles( (CPUCLK_FREQ / 1000)*__ms); }

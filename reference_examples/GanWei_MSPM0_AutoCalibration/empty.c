
#include "ti_msp_dl_config.h"
#include "No_Mcu_Ganv_Grayscale_Sensor_Config.h"
#include "delay.h"
#include "adc.h"
#include "led.h"
#include "uart.h"
#include "stdio.h"
#include "key.h"
#include "flash.h"
/*
 * =======================================================
 * 硬件引脚映射 (Hardware Pinout Map)
 * =======================================================
 * 
 * ----------------------------
 * LED控制引脚 (LED Control Pins)
 * ----------------------------
 * PB22  -> KEY_LED     // 按键状态指示灯
 * 
 * ----------------------------
 * 用户输入引脚 (User Input Pin)
 * ----------------------------
 * PB21  -> KEY         // 用户按键输入
 * 
 * ----------------------------
 * 无MCU传感器接口
 * (Dedicated Sensor Interface)
 * ----------------------------
 * PA15  -> AD0         // 地址输入通道0
 * PA14  -> AD1         // 地址输入通道1
 * PA13  -> AD2         // 地址输入通道2
 * PA27  -> OUT         // 传感器模拟量输出信号(接入ADC)
 * 
 * =======================================================
 */



/*                  模拟量转数字量的滞回比较器(施密特触发器)示意图               
 *          /\
 *   Digital |     
 *           |
 *        0  |                 +----------------+--------------
 *           |                 |                |
 *           |                 |                |
 *           |                 |                |
 *           |                 |                |
 *           |                 |                |
 *           |                 |                |
 *           |                 |                |
 *           |                 |                |     
 *         1 |    -------------+----------------+
 *      -----+----------------------------------------------------------> analog
 *           |   0             1/3              2/3            1 
 *           |   黑            灰黑             灰白            白
 *               Calibrated    Gray             Gray           Calibrated
 *               black         black            white          white
 */



// 全局变量定义
unsigned short Anolog[8] = {0};    // 存储当前模拟量值的数组
unsigned short white[8] = {0};     // 存储白色校准值的数组 
unsigned short black[8] = {0};     // 存储黑色校准值的数组
unsigned short Normal[8];          // 归一化值数组

No_MCU_Sensor sensor;              // 传感器数据结构体
unsigned char Digtal;              // 数字输出值
unsigned char rx_buff[256]={0};
int main(void)
{
    // 初始化系统配置
    SYSCFG_DL_init();
    
    // 初始化LED
    LED_init();
    
    /* DMA配置 - 用于ADC数据传输 */
    // 设置DMA源地址(ADC存储器)
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC0->ULLMEM.MEMRES[0]);
    // 设置DMA目标地址(ADC_VALUE缓冲区)
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC_VALUE[0]);		
    // 使能DMA通道
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);;
    // 启动ADC转换
    DL_ADC12_startConversion(ADC_VOLTAGE_INST);
    
    // 使能按键中断
    NVIC_EnableIRQ(GRAY_IN_INT_IRQN);
		
	
    // 从Flash存储器读取校准值
    readWhiteFromFlash();  // 读取白色校准值
    readBlackFromFlash();  // 读取黑色校准值

    // 使用校准值初始化传感器
    No_MCU_Ganv_Sensor_Init(&sensor, white, black);

    //无MCU灰度传感器硬件起振需要时间
    Tick_delay(100);
    state.value=KEY_IDLE;
		
    /* 主应用程序循环 */
    while (1) {
        if (state.value == KEY_IDLE||state.value == KEY_DISABLE||state.value == KEY_WAIT_LOSS ) {
            // 正常操作模式(非校准状态)
            
            // 执行无时基依赖的传感器任务
            No_Mcu_Ganv_Sensor_Task_Without_tick(&sensor);
            
            // 从传感器获取当前模拟量值
            Get_Anolog_Value(&sensor, Anolog);
            
            // 将模拟量转换为数字输出
            Digtal = Get_Digtal_For_User(&sensor);
					
        } else {
            // 校准模式 - 将数字输出置0，八路LED灯关闭
            Digtal = 0;
        }
				
        // 处理按键输入
        Key_Process();

			sprintf((char *)rx_buff,"Digtal %d-%d-%d-%d-%d-%d-%d-%d\r\n",(Digtal>>0)&0x01,(Digtal>>1)&0x01,(Digtal>>2)&0x01,(Digtal>>3)&0x01,(Digtal>>4)&0x01,(Digtal>>5)&0x01,(Digtal>>6)&0x01,(Digtal>>7)&0x01);
			uart0_send_string((char *)rx_buff);
			memset(rx_buff,0,256);
			
			sprintf((char *)rx_buff,"Anolog %d-%d-%d-%d-%d-%d-%d-%d\r\n",Anolog[0],Anolog[1],Anolog[2],Anolog[3],Anolog[4],Anolog[5],Anolog[6],Anolog[7]);
			uart0_send_string((char *)rx_buff);
			memset(rx_buff,0,256);
			
        // 更新KEY和ERR LED的状态
        LED_KEY_Blink_Update();
    }
}
/**
 * @brief 外部中断处理函数（CLK和按键）
 * @note  检测到有效按键后设置key_pressed标志
 */
void GROUP1_IRQHandler(void)
{
	    // 读取Group1的中断寄存器并清除中断标志位
    uint32_t pending = DL_GPIO_getPendingInterrupt(GPIOB);
	
    if(pending == GRAY_IN_IN_KEY_IIDX){
            /* 防抖处理 */
            if ((Tick - last_key_time) < DEBOUNCE_TIME_MS) {
                return;
            }
            /* 确认按键按下 */
            if (DL_GPIO_readPins(GRAY_IN_PORT, GRAY_IN_IN_KEY_PIN) == 0) {
                key_pressed = 1;
								long_pressed_key_time=Tick;
                last_key_time = Tick;
            }
    }
}
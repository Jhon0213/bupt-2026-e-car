#include "key.h"
#include "led.h"
#include "delay.h"
#include "uart.h"
#include "No_Mcu_Ganv_Grayscale_Sensor_Config.h"
#include "flash.h"
#include <string.h>


/* 全局变量 */
volatile uint32_t long_pressed_key_time;
volatile uint32_t last_key_time = 0;  // 上次按键时间戳
volatile uint8_t key_pressed = 0;     // 按键按下标志
Key_State state;       // 当前按键状态机状态
uint8_t long_err_flag = 0;            // 长按错误标志

/* 外部变量声明 */
extern unsigned short Anolog[8];      // 模拟量采样数组
extern unsigned short black[8];       // 黑场校准数据
extern unsigned short white[8];       // 白场校准数据  
extern No_MCU_Sensor sensor;          // 传感器数据结构体


/**
 * @brief 校准数据有效性检查
 * @param white 白场数据数组
 * @param black 黑场数据数组
 * @note 确保每个通道的白场值不小于黑场值
 */
void adjust_white_black(unsigned short white[8], unsigned short black[8])
{
    for (int i = 0; i < 8; ++i) {
        if (white[i] < black[i]) {
            unsigned short tmp = white[i];
            white[i] = black[i];
            black[i] = tmp;
        }
    }
}

/**
 * @brief 按键状态机处理函数
 * @note 实现完整的校准流程状态机:
 *       1. 长按进入校准模式
 *       2. 黑场数据采集
 *       3. 白场数据采集
 *       4. 数据校验存储
 */
void Key_Process(void)
{
    static uint32_t last_blink_time = 0;  // 上次闪烁时间
    static uint8_t led_state = 0;         // LED状态缓存
    
    switch (state.value) {
        /* 空闲状态 - 等待长按进入校准模式 */
        case KEY_IDLE:
            if(key_pressed) {
								if(Tick - long_pressed_key_time>=LONG_PRESS_TIME_MS){ 
									/* 确认仍保持按下 */
									if (DL_GPIO_readPins(GRAY_IN_PORT, GRAY_IN_IN_KEY_PIN) == 0) {
											LED_KEY_Blink_SetInterval(FAST_BLINK_INTERVAL);
											LED_KEY_Blink_Start();
											state.value = KEY_WAIT_LOSS;
											last_blink_time = Tick;
									}
								}
            }
            break;
            
        /* 等待松开按键状态 */
        case KEY_WAIT_LOSS:
            if (DL_GPIO_readPins(GRAY_IN_PORT, GRAY_IN_IN_KEY_PIN) == 0) {
                /* 检查是否超时 */
                if(Tick-last_blink_time >= MAX_PRESS_TIME_MS) {
                    state.value = KEY_LONG_ERROR;
                }
                Tick_delay(1);
            } else {
                /* 进入黑场校准准备状态 */
                LED_KEY_Blink_SetInterval(SLOW_BLINK_INTERVAL);
                state.value = KEY_BLACK_CHECK;
            }
            break;
            
        /* 黑场校准状态 */
        case KEY_BLACK_CHECK:
            if(key_pressed) {
                key_pressed = 0;
                Tick_delay(50);
                
                /* 确认按键按下 */
                if (DL_GPIO_readPins(GRAY_IN_PORT, GRAY_IN_IN_KEY_PIN) == 0) {
                    
                    /* 采集黑场数据 */
                    Get_Anolog_Value(&sensor, Anolog);
                    memcpy(black, Anolog, 8 * sizeof(unsigned short));
                    
                    /* 进入黑场确认状态 */
                    LED_KEY_Blink_SetInterval(FAST_BLINK_INTERVAL);
                    state.value = KEY_WAIT_BLACK_BLINK;
                    last_blink_time = Tick;
                }
            }
            break;
            
        /* 黑场确认状态 */
        case KEY_WAIT_BLACK_BLINK:
            if(Tick-last_blink_time >= 1000) {
                LED_KEY_Blink_SetInterval(SLOW_BLINK_INTERVAL);
                state.value = KEY_WHITE_CHECK;
            }
            break;
            
        /* 白场校准状态 */
        case KEY_WHITE_CHECK:
            if(key_pressed) {
                key_pressed = 0;
                Tick_delay(50);
                
                /* 确认按键按下 */
                if (DL_GPIO_readPins(GRAY_IN_PORT, GRAY_IN_IN_KEY_PIN) == 0) {
                    
                    /* 采集白场数据 */
                    Get_Anolog_Value(&sensor, Anolog);
                    memcpy(white, Anolog, 8 * sizeof(unsigned short));
                    
                    /* 校验并存储校准数据 */
                    adjust_white_black(white, black);
                    writeToFlash();
                    Tick_delay(100);
                    No_MCU_Ganv_Sensor_Init(&sensor, white, black);
                    Tick_delay(100);
                    
                    /* 进入白场确认状态 */
                    LED_KEY_Blink_SetInterval(FAST_BLINK_INTERVAL);
                    state.value = KEY_WAIT_WHITE_BLINK;
                    last_blink_time = Tick;
                }
            }
            break;
            
        /* 白场确认状态 */
        case KEY_WAIT_WHITE_BLINK:
            if(Tick-last_blink_time >= 1000) {
                LED_KEY_Blink_Stop();
                state.value = KEY_IDLE;
            }
            break;
            
        /* 长按错误状态 */
        case KEY_LONG_ERROR:
            LED_KEY_Blink_Stop();
            long_err_flag = 1;
            state.value = KEY_DISABLE;
            break;
            
        /* 过曝错误状态 */
        case KEY_LIGHT_ERROR:
            LED_KEY_Blink_Stop();
            break;
				case KEY_DISABLE:
					break;
    }
}
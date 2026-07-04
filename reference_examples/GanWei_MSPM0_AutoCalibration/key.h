#ifndef KEY_H
#define KEY_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
/* 按键相关宏定义 */
#define DEBOUNCE_TIME_MS     20    // 按键防抖时间(ms)
#define LONG_PRESS_TIME_MS   4000  // 长按判定时间(ms)
#define FAST_BLINK_INTERVAL  100   // 快闪间隔(ms)
#define SLOW_BLINK_INTERVAL  500   // 慢闪间隔(ms)
#define MAX_PRESS_TIME_MS    11000 // 最大允许按下时间(ms)-4000(ms)
typedef struct {
    int value;
} Key_State;

#define KEY_IDLE          0
#define KEY_WAIT_LOSS     1
#define KEY_BLACK_CHECK   2
#define KEY_WAIT_BLACK_BLINK 3
#define KEY_WHITE_CHECK   4
#define KEY_WAIT_WHITE_BLINK 5
#define KEY_LONG_ERROR    6
#define KEY_LIGHT_ERROR   7
#define KEY_DISABLE       8
extern volatile uint32_t long_pressed_key_time;//长按时间检测
extern volatile uint32_t last_key_time;  // 上次按键时间戳
extern volatile uint8_t key_pressed;     // 按键按下标志
extern uint8_t long_err_flag;
extern Key_State state;
extern volatile uint8_t key_pressed;
void Key_Process(void);
#endif // LED_H

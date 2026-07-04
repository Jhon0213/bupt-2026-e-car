#include "led.h"
#include <stddef.h>

/* LED引脚映射表 */
static const uint32_t LED_PINS[LED_COUNT] = {
    LED_KEY_PIN,    // LED_KEY
};

/* KEY灯闪烁控制结构体 */
typedef struct {
    uint32_t lastToggleTime;  // 上次状态切换时间戳
    uint32_t interval;        // 闪烁间隔(ms)
    uint8_t isActive;         // 闪烁使能标志
} LED_KEY_Blink_Control;
/* 静态全局变量 */
static LED_KEY_Blink_Control keyBlinkCtrl = {0};
/**
 * @brief LED模块初始化
 * @note 初始化后默认关闭所有LED
 */
void LED_init(void)
{
    // GPIO初始化已在SYSCFG_DL_GPIO_init中完成
    LED_setAll(0x00);  
}

/**
 * @brief 关闭指定LED
 * @param num LED编号(LED_Number枚举值)
 */
void LED_off(LED_Number num)
{
    if (num >= LED_COUNT) return;
    DL_GPIO_clearPins(GPIOB, LED_PINS[num]);
}

/**
 * @brief 点亮指定LED  
 * @param num LED编号(LED_Number枚举值)
 */
void LED_on(LED_Number num)
{
    if (num >= LED_COUNT) return;
    DL_GPIO_setPins(GPIOB, LED_PINS[num]);
}

/**
 * @brief 切换指定LED状态
 * @param num LED编号(LED_Number枚举值) 
 */
void LED_toggle(LED_Number num)
{
    if (num >= LED_COUNT) return;
    DL_GPIO_togglePins(GPIOB, LED_PINS[num]);
}

/**
 * @brief 批量控制所有LED状态
 * @param state 位掩码(bit0对应LED0, bit1对应LED1...)
 */
void LED_setAll(uint16_t state)
{
    uint32_t pinsToClear = 0;
    uint32_t pinsToSet = 0;
    
    for (LED_Number i = 0; i < LED_COUNT; i++) {
        if (state & (1 << i)) {
            pinsToSet |= LED_PINS[i];   // 需要点亮的LED
        } else {
            pinsToClear |= LED_PINS[i]; // 需要熄灭的LED
        }
    }
    
    DL_GPIO_clearPins(GPIOB, pinsToClear);
    DL_GPIO_setPins(GPIOB, pinsToSet);
}

/**
 * @brief 根据数字量控制前8路LED
 * @param digital 8位数据(每位对应一个LED状态)
 */
void LED_setByDigital(unsigned char digital)
{
    // 只控制前8个LED(LED_0到LED_7)
    for (uint8_t i = 0; i < 8; i++) {
        (digital & (1 << i)) ? LED_on(i) : LED_off(i);
    }
}

/******************** KEY灯闪烁控制 ********************/

/**
 * @brief 初始化KEY灯闪烁功能
 * @param interval 闪烁间隔时间(ms)
 */
void LED_KEY_Blink_Init(uint32_t interval)
{
    keyBlinkCtrl.interval = interval;
    keyBlinkCtrl.lastToggleTime = Tick;
    keyBlinkCtrl.isActive = 1;
    LED_off(LED_KEY);
}

/**
 * @brief 更新KEY灯闪烁状态(需在主循环调用)
 */
void LED_KEY_Blink_Update(void)
{
    if (!keyBlinkCtrl.isActive) return;
    
    if ((Tick - keyBlinkCtrl.lastToggleTime) >= keyBlinkCtrl.interval) {
        LED_toggle(LED_KEY);
        keyBlinkCtrl.lastToggleTime = Tick;
    }
}

/**
 * @brief 设置KEY灯闪烁间隔
 * @param newInterval 新的闪烁间隔(ms)
 */
void LED_KEY_Blink_SetInterval(uint32_t newInterval)
{
    keyBlinkCtrl.interval = newInterval;
}

/**
 * @brief 停止KEY灯闪烁
 */
void LED_KEY_Blink_Stop(void)
{
    keyBlinkCtrl.isActive = 0;
    LED_off(LED_KEY);
}

/**
 * @brief 开始KEY灯闪烁
 */
void LED_KEY_Blink_Start(void)
{
    keyBlinkCtrl.isActive = 1;
    keyBlinkCtrl.lastToggleTime = Tick;
}
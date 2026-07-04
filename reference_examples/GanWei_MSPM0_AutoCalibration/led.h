#ifndef LED_H
#define LED_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include "delay.h"
/**
 * @brief LED ��Ŷ���
 */
typedef enum {
    LED_KEY,
    LED_COUNT
} LED_Number;
void configure_QualityCheck_GPIO(void);
void restore_QualityCheck_GPIO(void);
/**
 * @brief ��ʼ�� LED ����
 */
void LED_init(void);

/**
 * @brief ��ָ�� LED
 * @param num LED ���
 */
void LED_on(LED_Number num);

/**
 * @brief �ر�ָ�� LED
 * @param num LED ���
 */
void LED_off(LED_Number num);

/**
 * @brief �л�ָ�� LED ״̬
 * @param num LED ���
 */
void LED_toggle(LED_Number num);

/**
 * @brief �������� LED ״̬
 * @param state ÿ��λ��Ӧһ�� LED ��״̬ (bit0: LED0, bit1: LED1, ...)
 */
void LED_setAll(uint16_t state);

void LED_setByDigital(unsigned char digital);

void LED_KEY_Blink_Update(void);

void LED_KEY_Blink_SetInterval(uint32_t newInterval);

void LED_KEY_Blink_Stop(void);

void LED_KEY_Blink_Start(void);

/**
 * @brief ��ʼ��ERR����˸����
 * @param interval ��˸���ʱ��(ms)
 */
void LED_ERR_Blink_Init(uint32_t interval);

/**
 * @brief ����ERR����˸״̬(������ѭ������)
 */
void LED_ERR_Blink_Update(void);

/**
 * @brief �����µ���˸���
 * @param newInterval �µ���˸���ʱ��(ms)
 */
void LED_ERR_Blink_SetInterval(uint32_t newInterval);

/**
 * @brief ֹͣERR����˸
 */
void LED_ERR_Blink_Stop(void);

/**
 * @brief ��ʼERR����˸
 */
void LED_ERR_Blink_Start(void);

/**
 * @brief ���ERR����˸״̬
 * @return 1-���� 0-δ����
 */
uint8_t LED_ERR_Blink_IsActive(void);
#endif // LED_H

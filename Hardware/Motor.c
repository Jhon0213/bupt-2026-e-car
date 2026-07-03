#include "Motor.h"

void Motor_Init(void)
{
	/* AIN/BIN 驱动板正转测试版本
	 * PA15 -> AIN1，对应 left PWM
	 * PA27 -> AIN2，当前固定低电平
	 * PA9  -> BIN1，对应 right PWM
	 * PA25 -> BIN2，当前固定低电平
	 * 当前仅验证正转，暂不支持真正反转逻辑。
	 */
	DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
	DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
	DL_TimerA_startCounter(MotorPWM_INST);
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, 0, GPIO_MotorPWM_C2_IDX);
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, 0, GPIO_MotorPWM_C1_IDX);
}

void move(int left, int right)
{
	int left_pwm = left;
	int right_pwm = right;

	/* AIN/BIN 驱动板正转测试版本：暂不做真正反转，只将负值取绝对值。 */
	if (left_pwm < 0) {
		left_pwm = -left_pwm;
	}
	if (right_pwm < 0) {
		right_pwm = -right_pwm;
	}

	/* PWM 限幅，避免超过定时器周期。 */
	if (left_pwm > 500) {
		left_pwm = 500;
	}
	if (right_pwm > 500) {
		right_pwm = 500;
	}

	/* 当前始终保持 AIN2 / BIN2 为低电平，仅通过 AIN1 / BIN1 的 PWM 做正转测试。 */
	DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
	DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);

	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, left_pwm, GPIO_MotorPWM_C2_IDX);
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, right_pwm, GPIO_MotorPWM_C1_IDX);
}


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

	/* 当前接线：
	 * PA15 -> AIN1，对应 GPIO_MotorPWM_C2_IDX
	 * PA27 -> AIN2，对应 Motor2_M2PIN_27_PIN
	 * PA9  -> BIN1，对应 GPIO_MotorPWM_C1_IDX
	 * PA25 -> BIN2，对应 Motor1_M1PIN_25_PIN
	 * 当前软件修正：左轮正方向正常；右轮物理方向相反，因此在 move() 中对右轮方向做软件反向。
	 */
	if (left_pwm < 0) {
		left_pwm = -left_pwm;
	}
	if (right_pwm < 0) {
		right_pwm = -right_pwm;
	}

	if (left_pwm > 500) {
		left_pwm = 500;
	}
	if (right_pwm > 500) {
		right_pwm = 500;
	}

	if (left > 0) {
		DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
	} else if (left < 0) {
		DL_GPIO_setPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
	} else {
		left_pwm = 0;
		DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
	}

	if (right > 0) {
		DL_GPIO_setPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
	} else if (right < 0) {
		DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
	} else {
		right_pwm = 0;
		DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
	}

	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, left_pwm, GPIO_MotorPWM_C2_IDX);
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, right_pwm, GPIO_MotorPWM_C1_IDX);
}


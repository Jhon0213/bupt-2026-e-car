#include "Motor.h"

#define MOTOR_PWM_MAX 500

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
	int left_compare = 0;
	int right_compare = 0;

	/* Current wiring:
	 * PA15 -> AIN1, left PWM, GPIO_MotorPWM_C2_IDX
	 * PA27 -> AIN2, left direction/control, Motor2_M2PIN_27_PIN
	 * PA9  -> BIN1, right PWM, GPIO_MotorPWM_C1_IDX
	 * PA25 -> BIN2, right direction/control, Motor1_M1PIN_25_PIN
	 * Right forward uses BIN2 = 1, so PWM compare must be compensated as
	 * MOTOR_PWM_MAX - right_pwm.
	 * Left backward uses AIN2 = 1, so PWM compare must be compensated as
	 * MOTOR_PWM_MAX - left_pwm.
	 */
	if (left_pwm < 0) {
		left_pwm = -left_pwm;
	}
	if (right_pwm < 0) {
		right_pwm = -right_pwm;
	}

	if (left_pwm > MOTOR_PWM_MAX) {
		left_pwm = MOTOR_PWM_MAX;
	}
	if (right_pwm > MOTOR_PWM_MAX) {
		right_pwm = MOTOR_PWM_MAX;
	}

	if (left > 0) {
		DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
		left_compare = left_pwm;
	} else if (left < 0) {
		DL_GPIO_setPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
		left_compare = MOTOR_PWM_MAX - left_pwm;
	} else {
		DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
		left_compare = 0;
	}

	if (right > 0) {
		DL_GPIO_setPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
		right_compare = MOTOR_PWM_MAX - right_pwm;
	} else if (right < 0) {
		DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
		right_compare = right_pwm;
	} else {
		DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
		right_compare = 0;
	}

	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, left_compare, GPIO_MotorPWM_C2_IDX);
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST, right_compare, GPIO_MotorPWM_C1_IDX);
}


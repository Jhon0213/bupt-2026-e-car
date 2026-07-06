#include "control.h"

#define PWM_MAX 200
#define PWM_MIN -200
#define DEAD_ZONE 16

float Kp_angle = 6.55f, Ki_angle = 0.03f,Kd_angle = 0.00f;
float Kp_gyro=0.450f,Ki_gyro=0.025f,Kd_gyro=0.00f;
float angle_error = 0, angle_sum = 0,last_angle_error=0;
float gyro_error=0,gyro_sum = 0,last_gyro_error=0;

float limit(float val, float min_val, float max_val)
{
    if (val > max_val) return max_val;
    if (val < min_val) return min_val;
    return val;
}

/*

������yaw_control_with_gyro

���������������ǵĺ���Ǵ���PID���ƺ���

�⻷���ǶȻ�������Ŀ�꺽��ǣ�����������ٶ�

�ڻ������ٶȻ��������������ٶȣ����PWM���ٿ�����

������

base_speed - ����ǰ���ٶȣ�ռ�ձȻ�PWMֵ��

target_yaw - Ŀ�꺽��ǣ��ȣ���Χ[-180, 180]��

������

Yaw() ��ȡ��ǰ����ǣ��ȣ�

GyroZ() ��ȡ��ǰZ����ٶȣ���/�룩

move(L, R) ����������PWM���

limit(val, min, max) �޷�����

ȫ�ֱ�����

angle_error, angle_sum �⻷�Ƕ���������

gyro_error, gyro_sum �ڻ����ٶ���������

Kp_angle, Ki_angle �⻷PID����

Kp_gyro, Ki_gyro �ڻ�PID����

DEAD_ZONE ��������ֵ

PWM_MIN, PWM_MAX PWM����޷�ֵ

����ԭ���

���㵱ǰ������Ŀ�������һ����[-180��, 180��]
�⻷PI���������ݽǶ��������������ٶ�
�ڻ�PI���������ݽ��ٶ�������ת��PWM
ͨ�����ٿ��ƣ����ּ��١����ּ��ٻ�֮��ʵ��ת��
�������������Կ˷���е��Ħ�����Ƽ�ʹ��Ť�ش�����С����λС�ĵ����
*/
void yaw_control_with_gyro(float base_speed, float target_yaw)
{
    // ============ �⻷���Ƕ�PID������������ٶȣ�============
    // === 1. ��ȡ��ǰ�Ƕ� ===
    float yaw_now = Yaw();

    // === 2. ����Ƕ���� ===
    angle_error = target_yaw - yaw_now;

    // �Ƕ�����һ���� [-180, 180]
    if (angle_error > 180)  angle_error -= 360;
    if (angle_error < -180) angle_error += 360;

    // === 3. �⻷�����޷� ===
    #define ANGLE_SUM_MAX 300.0f
	
    angle_sum += angle_error;
    if (angle_sum >  ANGLE_SUM_MAX) angle_sum =  ANGLE_SUM_MAX;
    if (angle_sum < -ANGLE_SUM_MAX) angle_sum = -ANGLE_SUM_MAX;

    // === 4. �⻷PID �� ����������ٶ� ===
    float desired_gyro = Kp_angle * angle_error      // �Ƕȱ�����
                       + Ki_angle * angle_sum;        // �ǶȻ�����
    // ע�⣺�⻷һ�㲻ʹ��΢������Ҫ�ÿ��Լӣ�
    // + Kd_angle * (angle_error - last_angle_error);
		// === 12. ����Ƕ�������⻷Ҫ��΢�֣�===
    //last_angle_error = angle_error;

    // ============ �ڻ������ٶ�PID�������������ٶȣ�============
    // === 5. ��ȡ��ǰ���ٶȣ������ǣ�===
    float gyro_now = GyroZ();;  // ���� GyroZ() �����ȫ�ֱ��� gz

    // === 6. ������ٶ���� ===
    gyro_error = desired_gyro - gyro_now;

    // === 7. �ڻ������޷� ===
    #define GYRO_SUM_MAX 80.0f          //��Ҫ����400
		
    gyro_sum += gyro_error;
    if (gyro_sum >  GYRO_SUM_MAX) gyro_sum =  GYRO_SUM_MAX;
    if (gyro_sum < -GYRO_SUM_MAX) gyro_sum = -GYRO_SUM_MAX;

    // === 8. �ڻ�PID �� ���PWM���� ===
    float turn_pwm = Kp_gyro * gyro_error            // ���ٶȱ�����
                   + Ki_gyro * gyro_sum    ;          // ���ٶȻ�����
//                   + Kd_gyro * (gyro_error - last_gyro_error); // ���ٶ�΢����

//    // ���汾�ν��ٶ��������´�΢�֣�
//    last_gyro_error = gyro_error;

    // === 9. �������� ===
    if (turn_pwm > 0.1f) {
        turn_pwm += DEAD_ZONE;
    } else if (turn_pwm < -0.1f) {
        turn_pwm -= DEAD_ZONE;
    }

    // === 10. ������PWM���� ===
    float pwm_left  = base_speed - turn_pwm;
    float pwm_right = base_speed + turn_pwm;

    // === 11. PWM�޷� ===
    pwm_left  = limit(pwm_left, PWM_MIN, PWM_MAX);
    pwm_right = limit(pwm_right, PWM_MIN, PWM_MAX);

    move(pwm_left, pwm_right);
}

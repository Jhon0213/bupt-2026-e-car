#include "control.h"

// === PWM 限幅范围
#define PWM_MAX 200
#define PWM_MIN -200

// === 死区补偿值（电机刚好能动的最小 PWM）===
#define DEAD_ZONE 16

// === PID 参数（可调）===
float Kp_angle = 6.55f, Ki_angle = 0.03f,Kd_angle = 0.00f;    //外环参数
float Kp_gyro=0.450f,Ki_gyro=0.025f,Kd_gyro=0.00f;          		//内环参数

// === 控制变量 ===
float angle_error = 0, angle_sum = 0,last_angle_error=0;   			// 角度积分（用于外环）
float gyro_error=0,gyro_sum = 0,last_gyro_error=0;  						// 角速度积分（用于内环）



// === 限幅函数 ===
float limit(float val, float min_val, float max_val)
{
    if (val > max_val) return max_val;
    if (val < min_val) return min_val;
    return val;
}

/*

函数：yaw_control_with_gyro

描述：基于陀螺仪的航向角串级PID控制函数

外环：角度环，输入目标航向角，输出期望角速度

内环：角速度环，输入期望角速度，输出PWM差速控制量

参数：

base_speed - 基础前进速度（占空比或PWM值）

target_yaw - 目标航向角（度，范围[-180, 180]）

依赖：

Yaw() 获取当前航向角（度）

GyroZ() 获取当前Z轴角速度（度/秒）

move(L, R) 设置左右轮PWM输出

limit(val, min, max) 限幅函数

全局变量：

angle_error, angle_sum 外环角度误差、误差积分

gyro_error, gyro_sum 内环角速度误差、误差积分

Kp_angle, Ki_angle 外环PID参数

Kp_gyro, Ki_gyro 内环PID参数

DEAD_ZONE 死区补偿值

PWM_MIN, PWM_MAX PWM输出限幅值

工作原理：

计算当前航向与目标的误差，归一化至[-180°, 180°]
外环PI控制器根据角度误差输出期望角速度
内环PI控制器根据角速度误差输出转向PWM
通过差速控制（左轮减速、右轮加速或反之）实现转向
加入死区补偿以克服机械静摩擦，推荐使用扭矩大，死区小，虚位小的电机。
*/
void yaw_control_with_gyro(float base_speed, float target_yaw)
{
    // ============ 外环：角度PID（输出期望角速度）============
    // === 1. 获取当前角度 ===
    float yaw_now = Yaw();

    // === 2. 计算角度误差 ===
    angle_error = target_yaw - yaw_now;

    // 角度误差归一化到 [-180, 180]
    if (angle_error > 180)  angle_error -= 360;
    if (angle_error < -180) angle_error += 360;

    // === 3. 外环积分限幅 ===
    #define ANGLE_SUM_MAX 300.0f
	
    angle_sum += angle_error;
    if (angle_sum >  ANGLE_SUM_MAX) angle_sum =  ANGLE_SUM_MAX;
    if (angle_sum < -ANGLE_SUM_MAX) angle_sum = -ANGLE_SUM_MAX;

    // === 4. 外环PID → 输出期望角速度 ===
    float desired_gyro = Kp_angle * angle_error      // 角度比例项
                       + Ki_angle * angle_sum;        // 角度积分项
    // 注意：外环一般不使用微分项，如果要用可以加：
    // + Kd_angle * (angle_error - last_angle_error);
		// === 12. 保存角度误差（如果外环要用微分）===
    //last_angle_error = angle_error;

    // ============ 内环：角速度PID（跟踪期望角速度）============
    // === 5. 获取当前角速度（陀螺仪）===
    float gyro_now = GyroZ();;  // 假设 GyroZ() 会更新全局变量 gz

    // === 6. 计算角速度误差 ===
    gyro_error = desired_gyro - gyro_now;

    // === 7. 内环积分限幅 ===
    #define GYRO_SUM_MAX 80.0f          //不要超过400
		
    gyro_sum += gyro_error;
    if (gyro_sum >  GYRO_SUM_MAX) gyro_sum =  GYRO_SUM_MAX;
    if (gyro_sum < -GYRO_SUM_MAX) gyro_sum = -GYRO_SUM_MAX;

    // === 8. 内环PID → 输出PWM差速 ===
    float turn_pwm = Kp_gyro * gyro_error            // 角速度比例项
                   + Ki_gyro * gyro_sum    ;          // 角速度积分项
//                   + Kd_gyro * (gyro_error - last_gyro_error); // 角速度微分项

//    // 保存本次角速度误差（用于下次微分）
//    last_gyro_error = gyro_error;

    // === 9. 死区补偿 ===
    if (turn_pwm > 0.1f) {
        turn_pwm += DEAD_ZONE;
    } else if (turn_pwm < -0.1f) {
        turn_pwm -= DEAD_ZONE;
    }

    // === 10. 左右轮PWM计算 ===
    float pwm_left  = base_speed - turn_pwm;
    float pwm_right = base_speed + turn_pwm;

    // === 11. PWM限幅 ===
    pwm_left  = limit(pwm_left, PWM_MIN, PWM_MAX);
    pwm_right = limit(pwm_right, PWM_MIN, PWM_MAX);

    move(pwm_left, pwm_right);
}



// ==================== 左轮速度环变量 ====================
float speed_error_left = 0.0f;
float speed_last_error_left = 0.0f;
float speed_integral_left = 0.0f;

// 左轮PID参数（可单独调）
float Kp_speed_left = 8.8f;
float Ki_speed_left = 0.0f;
float Kd_speed_left = 0.0f;

// ==================== 右轮速度环变量 ====================
float speed_error_right = 0.0f;
float speed_last_error_right = 0.0f;
float speed_integral_right = 0.0f;

// 右轮PID参数（可单独调）
float Kp_speed_right = 0.0f;
float Ki_speed_right = 0.0f;
float Kd_speed_right = 0.0f;

// 公共限幅
#define SPEED_INTEGRAL_MAX 400.0f
#define SPEED_OUTPUT_MAX   400.0f
#define SPEED_DEAD_ZONE    10.0f

// ==================== 左轮速度环 ====================
float Speed_Loop_Left(float target_speed)
{
    float dt = 0.01f;
    
    float current_speed = motor_1.speed;
    speed_error_left = target_speed - current_speed;
    
    // 使用左轮的Kp
    float p_out = Kp_speed_left * speed_error_left;
    
    // 使用左轮的Ki
    speed_integral_left += Ki_speed_left * speed_error_left * dt;
    if(speed_integral_left > SPEED_INTEGRAL_MAX) 
        speed_integral_left = SPEED_INTEGRAL_MAX;
    if(speed_integral_left < -SPEED_INTEGRAL_MAX) 
        speed_integral_left = -SPEED_INTEGRAL_MAX;
    float i_out = speed_integral_left;
    
    // 使用左轮的Kd
    float d_out = Kd_speed_left * (speed_error_left - speed_last_error_left) / dt;
    speed_last_error_left = speed_error_left;
    
    float output = p_out + i_out + d_out;
    
    if(output > SPEED_OUTPUT_MAX) output = SPEED_OUTPUT_MAX;
    if(output < -SPEED_OUTPUT_MAX) output = -SPEED_OUTPUT_MAX;
    
    if(output > 0 && output < SPEED_DEAD_ZONE) output = SPEED_DEAD_ZONE;
    if(output < 0 && output > -SPEED_DEAD_ZONE) output = -SPEED_DEAD_ZONE;
    
    return output;
}

// ==================== 右轮速度环 ====================
float Speed_Loop_Right(float target_speed)
{
    float dt = 0.01f;
    
    float current_speed = motor_2.speed;
    speed_error_right = target_speed - current_speed;
    
    // 使用右轮的Kp
    float p_out = Kp_speed_right * speed_error_right;
    
    // 使用右轮的Ki
    speed_integral_right += Ki_speed_right * speed_error_right * dt;
    if(speed_integral_right > SPEED_INTEGRAL_MAX) 
        speed_integral_right = SPEED_INTEGRAL_MAX;
    if(speed_integral_right < -SPEED_INTEGRAL_MAX) 
        speed_integral_right = -SPEED_INTEGRAL_MAX;
    float i_out = speed_integral_right;
    
    // 使用右轮的Kd
    float d_out = Kd_speed_right * (speed_error_right - speed_last_error_right) / dt;
    speed_last_error_right = speed_error_right;
    
    float output = p_out + i_out + d_out;
    
    if(output > SPEED_OUTPUT_MAX) output = SPEED_OUTPUT_MAX;
    if(output < -SPEED_OUTPUT_MAX) output = -SPEED_OUTPUT_MAX;
    
    if(output > 0 && output < SPEED_DEAD_ZONE) output = SPEED_DEAD_ZONE;
    if(output < 0 && output > -SPEED_DEAD_ZONE) output = -SPEED_DEAD_ZONE;
    
    return output;
}

// ==================== 速度环主函数 ====================
void Speed_Control(float target_left, float target_right)
{
    float pwm_left = Speed_Loop_Left(target_left);
    float pwm_right = Speed_Loop_Right(target_right);
    
    move(pwm_left, pwm_right);
}




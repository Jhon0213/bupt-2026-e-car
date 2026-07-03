#include "Motor.h"

void Motor_Init(void)
{
	DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);
	DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);
  DL_TimerA_startCounter(MotorPWM_INST);
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST,0,GPIO_MotorPWM_C2_IDX );
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST,0,GPIO_MotorPWM_C1_IDX);
}

void move(int left,int right)
{
	if(left>0)
		{	 DL_GPIO_setPins(Motor2_PORT, Motor2_M2PIN_27_PIN);		}
	else if(left<0)
		{DL_GPIO_clearPins(Motor2_PORT, Motor2_M2PIN_27_PIN);left=-left;}
	else
		{DL_TimerA_setCaptureCompareValue(MotorPWM_INST,0,GPIO_MotorPWM_C2_IDX);}
	
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST,left,GPIO_MotorPWM_C2_IDX );
		
		
	if(right>0)
		{DL_GPIO_clearPins(Motor1_PORT, Motor1_M1PIN_25_PIN);	}
	else if(right<0)
		{DL_GPIO_setPins(Motor1_PORT, Motor1_M1PIN_25_PIN);right=-right;}
	else  
		{DL_TimerA_setCaptureCompareValue(MotorPWM_INST,0,GPIO_MotorPWM_C1_IDX );}
		
	DL_TimerA_setCaptureCompareValue(MotorPWM_INST,right,GPIO_MotorPWM_C1_IDX );
}


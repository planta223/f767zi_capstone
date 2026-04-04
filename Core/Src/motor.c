/*
 * motor.c
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#include "motor.h"
#include "tim.h"
#include "config.h"

/* =========================================
 * static 변수 및 함수
 * ========================================= */
// 1. 비정상 pwm 값 강제 정상화 로직

static uint16_t Motor_AbsClampPWM(int32_t pwm)
{
	if (pwm < 0) pwm = -pwm;
	if (pwm > MOTOR_PWM_MAX) pwm = MOTOR_PWM_MAX;
	return (uint16_t)pwm;
}



/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. 모터 출력 0

void Motor_Init(void)
{
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

    Motor_StopAll();
}



/* =========================================
 * global 함수
 * ========================================= */
// 1. 왼쪽 모터 구동 (PWM, DIR)
// 2. 오른쪽 모터 구동 (PWM, DIR)
// 3. 양쪽 모터 구동 (LEFT/RIGHT, PWM, DIR)
// 4. 양쪽 모터 정지 (coast stop)

void Motor_Left_SetCommand(int16_t cmd)
{
	uint16_t pwm  = Motor_AbsClampPWM(cmd);

	if (cmd >= 0)
	{
		HAL_GPIO_WritePin(left_DIR_GPIO_Port, left_DIR_Pin, GPIO_PIN_RESET); // DIR 반전
	}
	else
	{
	    HAL_GPIO_WritePin(left_DIR_GPIO_Port, left_DIR_Pin, GPIO_PIN_SET);
	}

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm);
}

void Motor_Right_SetCommand(int16_t cmd)
{
	uint16_t pwm  = Motor_AbsClampPWM(cmd);

	if (cmd >= 0)
	{
	    HAL_GPIO_WritePin(right_DIR_GPIO_Port, right_DIR_Pin, GPIO_PIN_SET);
	}
	else
	{
	    HAL_GPIO_WritePin(right_DIR_GPIO_Port, right_DIR_Pin, GPIO_PIN_RESET);
	}

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm);
}

void Motor_Both_SetCommand(int16_t left_cmd, int16_t right_cmd)
{
	Motor_Left_SetCommand(left_cmd);
	Motor_Right_SetCommand(right_cmd);
}

void Motor_StopAll(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
}

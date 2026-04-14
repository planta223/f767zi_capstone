/*
 * stepper.c
 *
 *  Created on: Apr 14, 2026
 *      Author: kyubeom
 */

#include "stepper.h"
#include "tim.h"
#include "config.h"

/* =========================================
 * static 변수 및 함수
 * ========================================= */
static volatile uint16_t slider_steps_remain = 0U;
static volatile uint8_t  slider_busy = 0U;
static volatile uint16_t arm_steps_remain = 0U;
static volatile uint8_t  arm_busy = 0U;

static uint16_t Stepper_AbsClampSteps(int16_t steps)
{
    if (steps < 0) steps = -steps;
    if (steps > STEPPER_STEPS_MAX)steps = STEPPER_STEPS_MAX;
    return (uint16_t)steps;
}



/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
void Stepper_Init(void)
{
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);   // slider_PUL
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);   // arm_PUL

    Stepper_StopAll();
}

/* =========================================
 * global 함수
 * ========================================= */
void Stepper_Slider_SetCommand(int16_t cmd)
{
    uint16_t steps = Stepper_AbsClampSteps(cmd);

    if ((cmd == 0) || (slider_busy == 1U))
    {
    	return;
    }

    if (cmd > 0)
    {
        HAL_GPIO_WritePin(slider_DIR_GPIO_Port, slider_DIR_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(slider_DIR_GPIO_Port, slider_DIR_Pin, GPIO_PIN_SET);
    }

    slider_steps_remain = steps;
    slider_busy = 1U;

    __HAL_TIM_SET_COUNTER(&htim5, 0U);
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, STEPPER_PULSE_DUTY);

    HAL_TIM_Base_Start_IT(&htim5); // 인터럽트 시작
}

void Stepper_Arm_SetCommand(int16_t cmd)
{
    uint16_t steps = Stepper_AbsClampSteps(cmd);

    if ((cmd == 0) || (arm_busy == 1U))
    {
        return;
    }

    if (cmd > 0)
    {
        HAL_GPIO_WritePin(arm_DIR_GPIO_Port, arm_DIR_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(arm_DIR_GPIO_Port, arm_DIR_Pin, GPIO_PIN_SET);
    }

    arm_steps_remain = steps;
    arm_busy = 1U;

    __HAL_TIM_SET_COUNTER(&htim8, 0U);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, STEPPER_PULSE_DUTY);

    HAL_TIM_Base_Start_IT(&htim8); // 인터럽트 시작
}


void Stepper_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM5) // slider
    {
        if (slider_busy == 1U)
        {
            if (slider_steps_remain > 0U)
            {
                slider_steps_remain--;
            }

            if (slider_steps_remain == 0U)
            {
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, 0U);
                HAL_TIM_Base_Stop_IT(&htim5); // 인터럽트 종료
                slider_busy = 0U;
            }
        }
    }
    else if (htim->Instance == TIM8) // arm
    {
        if (arm_busy == 1U)
        {
            if (arm_steps_remain > 0U)
            {
                arm_steps_remain--;
            }

            if (arm_steps_remain == 0U)
            {
                __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0U);
                HAL_TIM_Base_Stop_IT(&htim8); // 인터럽트 종료
                arm_busy = 0U;
            }
        }
    }
}

void Stepper_StopAll(void)
{
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, 0U);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0U);

    HAL_TIM_Base_Stop_IT(&htim5);
    HAL_TIM_Base_Stop_IT(&htim8);

    slider_steps_remain = 0U;
    slider_busy = 0U;
    arm_steps_remain = 0U;
    arm_busy = 0U;
}

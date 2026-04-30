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
static volatile uint16_t arm_steps_remain    = 0U;

static volatile StepperState_t stepper_state = STEPPER_IDLE;
static volatile uint8_t slider_busy = 0U;
static volatile uint8_t arm_busy    = 0U;

static volatile uint8_t slider_curr_idx      = 0U;
static volatile uint8_t arm_curr_idx         = 0U;
static volatile uint8_t slider_target_idx    = 0U; // 0,1,2,3
static volatile uint8_t arm_target_idx       = 0U; // 0,1,2

static volatile uint8_t startup_started = 0U;
static volatile uint8_t startup_slider_started = 0U;
static volatile uint8_t dropoff_done_latched = 0U;
static volatile uint8_t dropoff_target_id = 0U;

static uint16_t Stepper_AbsClampSteps(int32_t steps)
{
    if (steps < 0) steps = -steps;
    if (steps > STEPPER_STEPS_MAX)steps = STEPPER_STEPS_MAX;
    return (uint16_t)steps;
}

static uint8_t Stepper_TargetIdToIdx(uint8_t target_id)
{
    // target id 유효 범위 검사 (0~6) (실패시 오류반환)
    if (target_id > 6U)
    {
        return 0U;
    }

    // target id = 0 이면 init index 설정
    if (target_id == 0U)
    {
    	slider_target_idx = 0U;   // init
    	arm_target_idx    = 0U;   // init
        return 1U;
    }

    slider_target_idx = ((target_id - 1U) / 2U + 1);  // 1,2,3
    arm_target_idx    = ((target_id - 1U) % 2U + 1);  // 1,2
    return 1U;
}

static int32_t Slider_IdxToSteps(uint8_t idx)
{
    switch (idx)
    {
        case 0U: return 0;
        case 1U: return SLIDER_OFFSET_STEPS;
        case 2U: return SLIDER_OFFSET_STEPS + SLIDER_GAP_STEPS;
        case 3U: return SLIDER_OFFSET_STEPS + (2 * SLIDER_GAP_STEPS);
        default: return 0;
    }
}

static int32_t Arm_IdxToSteps(uint8_t idx)
{
    switch (idx)
    {
        case 0U: return 0;                  // center
        case 1U: return -ARM_SIDE_STEPS;    // left 70 deg
        case 2U: return  ARM_SIDE_STEPS;    // right 70 deg
        default: return 0;
    }
}

static int32_t Slider_CalcDelta(void)
{
    int32_t current_abs;
    int32_t target_abs;

    current_abs = Slider_IdxToSteps(slider_curr_idx);
    target_abs  = Slider_IdxToSteps(slider_target_idx);

    return (target_abs - current_abs);
}

static int32_t Arm_CalcDelta(void)
{
    int32_t current_abs;
    int32_t target_abs;

    current_abs = Arm_IdxToSteps(arm_curr_idx);
    target_abs  = Arm_IdxToSteps(arm_target_idx);

    return (target_abs - current_abs);

}


/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
void Stepper_Init(void)
{
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);   // slider_PUL
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);   // arm_PUL

    Stepper_StopAll();

    slider_curr_idx   = 0U;
    arm_curr_idx      = 0U;
    slider_target_idx = 0U;
    arm_target_idx    = 0U;

    slider_busy = 0U;
    arm_busy    = 0U;

    startup_started        = 0U;
    startup_slider_started = 0U;

    dropoff_done_latched = 0U;

    stepper_state = STEPPER_STARTUP;
}

/* =========================================
 * global 함수
 * ========================================= */
void Stepper_Update(void)
{
    int32_t delta;

    switch (stepper_state)
    {
        case STEPPER_IDLE:
            break;

        case STEPPER_STARTUP:
            /* arm init 시작 전 */
            if (startup_started == 0U)
            {
                startup_started = 1U;

                if (arm_curr_idx != 0U)
                {
                    arm_target_idx = 0U;
                    delta = Arm_CalcDelta();

                    if (delta != 0)
                    {
                        Stepper_Arm_SetCommand((int16_t)delta);
                        break;
                    }
                }

                arm_curr_idx = 0U;
            }

            /* arm init 완료 후 slider init 시작 */
            if ((arm_busy == 0U) && (startup_slider_started == 0U))
            {
                startup_slider_started = 1U;

                Stepper_Slider_SetCommand(-STEPPER_STARTUP_SLIDER_HOMING_STEPS);
                break;
            }

            /* slider init 완료 */
            if ((startup_slider_started == 1U) && (slider_busy == 0U))
            {
                slider_curr_idx   = 0U;
                slider_target_idx = 0U;
                arm_curr_idx      = 0U;
                arm_target_idx    = 0U;

                startup_started = 0U;
                startup_slider_started = 0U;
                stepper_state = STEPPER_IDLE;
            }
            break;

        case STEPPER_ARM_TO_INIT_PRE:
            if (arm_busy == 0U)
            {
                arm_curr_idx = 0U;

                delta = Slider_CalcDelta();
                if (delta != 0)
                {
                    Stepper_Slider_SetCommand((int16_t)delta);
                }

                stepper_state = STEPPER_SLIDER_TO_TARGET;
            }
            break;

        case STEPPER_SLIDER_TO_TARGET:
            if (slider_busy == 0U)
            {
                slider_curr_idx = slider_target_idx;

                if (dropoff_target_id == 0U)
                {
                    stepper_state = STEPPER_DONE;
                    break;
                }

                delta = Arm_CalcDelta();
                if (delta != 0)
                {
                    Stepper_Arm_SetCommand((int16_t)delta);
                }

                stepper_state = STEPPER_ARM_TO_TARGET;
            }
            break;

        case STEPPER_ARM_TO_TARGET:
            if (arm_busy == 0U)
            {
                arm_curr_idx = arm_target_idx;

                arm_target_idx = 0U;
                delta = Arm_CalcDelta();

                if (delta != 0)
                {
                    Stepper_Arm_SetCommand((int16_t)delta);
                    stepper_state = STEPPER_ARM_TO_INIT_POST;
                }
                else
                {
                    stepper_state = STEPPER_DONE;
                }
            }
            break;

        case STEPPER_ARM_TO_INIT_POST:
            if (arm_busy == 0U)
            {
                arm_curr_idx = 0U;
                stepper_state = STEPPER_DONE;
            }
            break;

        case STEPPER_DONE:
        	dropoff_done_latched = 1U;
            stepper_state = STEPPER_IDLE;
            break;

        default:
            stepper_state = STEPPER_IDLE;
            break;
    }
}

uint8_t Stepper_Dropoff_Start(uint8_t target_id)
{
    int32_t delta;

    // Dropoff 동작은 STEPPER_IDLE 상태일때만 진행
    if (stepper_state != STEPPER_IDLE)
    {
        return 0U;
    }

    // target_id 유효성 판단
    if (Stepper_TargetIdToIdx(target_id) == 0U)
    {
        return 0U;
    }

    dropoff_target_id = target_id;
    dropoff_done_latched = 0U;

    arm_target_idx = 0U;
    delta = Arm_CalcDelta();

    // 1. 먼저 arm 중앙 복귀
    if (delta != 0)
    {
        Stepper_Arm_SetCommand((int16_t)delta);
        stepper_state = STEPPER_ARM_TO_INIT_PRE;
    }
    // 2. 그다음 slider를 목표 위치로 이동 -> arm 목표 위치로 이동 -> arm 중앙 복귀 -> 동작 완료
    else
    {
        arm_curr_idx = 0U;

        delta = Slider_CalcDelta();
        if (delta != 0)
        {
            Stepper_Slider_SetCommand((int16_t)delta);
        }

        stepper_state = STEPPER_SLIDER_TO_TARGET;
    }

    return 1U;
}

// startup 중이든, dropoff 중이든, arm post-init 중이든 다 busy로 봅니다.
uint8_t Stepper_IsBusy(void)
{
    return (uint8_t)(stepper_state != STEPPER_IDLE);
}

uint8_t Stepper_GetAndClearDropoffDone(void)
{
    uint8_t ret = dropoff_done_latched;
    dropoff_done_latched = 0U;
    return ret;
}

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

void Stepper_StopAll(void)
{

    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, 0U);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0U);

    HAL_TIM_Base_Stop_IT(&htim5);
    HAL_TIM_Base_Stop_IT(&htim8);

    slider_busy = 0U;
    arm_busy    = 0U;

    startup_started        = 0U;
    startup_slider_started = 0U;

    stepper_state = STEPPER_IDLE;

    dropoff_done_latched = 0U;
    dropoff_target_id    = 0U;
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

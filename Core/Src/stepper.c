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

static volatile uint8_t dropoff_done_latched = 0U;
static volatile uint8_t dropoff_target_id = 0U;

static volatile uint32_t arm_hold_start_ms = 0U;
static volatile uint32_t slider_hold_start_ms = 0U;

static uint16_t Stepper_AbsClampSteps(int32_t steps)
{
    if (steps < 0) steps = -steps;
    if (steps > STEPPER_CMD_PULSES_MAX)steps = STEPPER_CMD_PULSES_MAX;
    return (uint16_t)steps;
}

static uint8_t Stepper_TargetIdToIdx(uint8_t target_id)
{
    // target id 유효 범위 검사 (0~DROPOFF_TARGET_MAX_ID) (실패시 오류반환)
    if (target_id > DROPOFF_TARGET_MAX_ID)
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
        case 1U: return SLIDER_OFFSET_PULSES;
        case 2U: return SLIDER_OFFSET_PULSES + SLIDER_GAP_PULSES;
        case 3U: return SLIDER_OFFSET_PULSES + (2 * SLIDER_GAP_PULSES);
        default: return 0;
    }
}

static int32_t Arm_IdxToSteps(uint8_t idx)
{
    switch (idx)
    {
        case 0U: return 0;                  // center
        case 1U: return -ARM_SIDE_PULSES;    // left 70 deg
        case 2U: return  ARM_SIDE_PULSES;    // right 70 deg
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

    dropoff_done_latched = 0U;
    dropoff_target_id    = 0U;

    arm_hold_start_ms = 0U;
    slider_hold_start_ms = 0U;

    stepper_state = STEPPER_IDLE;   // 부팅 직후 자동 homing 금지
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

        case STEPPER_SLIDER_MANUAL:
            /*
             * 버튼을 누르고 있는 동안 TIM5가 계속 pulse를 발생시킨다.
             * 버튼 release 시 Stepper_SliderJogStop()에서 정지한다.
             */
            break;

        case STEPPER_ARM_TO_INIT_PRE:
            if (arm_busy == 0U)
            {
                arm_curr_idx = 0U;

                delta = Slider_CalcDelta();
                if (delta != 0)
                {
                    Stepper_Slider_SetCommand(delta);
                }

                stepper_state = STEPPER_SLIDER_TO_TARGET;
            }
            break;

        case STEPPER_SLIDER_TO_TARGET:
            if (slider_busy == 0U)
            {
                slider_curr_idx = slider_target_idx;

                /*
                 * slider가 목표 위치에 도달한 뒤
                 * 바로 arm을 움직이지 않고 일정 시간 유지한다.
                 */
                slider_hold_start_ms = HAL_GetTick();
                stepper_state = STEPPER_SLIDER_HOLD_AT_TARGET;
            }
            break;

        case STEPPER_SLIDER_HOLD_AT_TARGET:
            if ((HAL_GetTick() - slider_hold_start_ms) >= SLIDER_DROPOFF_HOLD_MS)
            {
                if (dropoff_target_id == 0U)
                {
                    stepper_state = STEPPER_DONE;
                    break;
                }

                /*
                 * slider 이동 완료 후, dropoff target 기준 arm 목표 재설정
                 * Dropoff_Start()에서 arm_target_idx를 0으로 덮었기 때문에 여기서 복구해야 함.
                 */
                arm_target_idx = ((dropoff_target_id - 1U) % 2U + 1U);

                delta = Arm_CalcDelta();

                if (delta != 0)
                {
                    Stepper_Arm_SetCommand(delta);
                    stepper_state = STEPPER_ARM_TO_TARGET;
                }
                else
                {
                    stepper_state = STEPPER_DONE;
                }
            }
            break;

        case STEPPER_ARM_TO_TARGET:
            if (arm_busy == 0U)
            {
                arm_curr_idx = arm_target_idx;

                /*
                 * arm이 목표 하역 위치(+/-70 deg)에 도달한 뒤
                 * 바로 center 복귀하지 않고 일정 시간 유지한다.
                 */
                arm_hold_start_ms = HAL_GetTick();
                stepper_state = STEPPER_ARM_HOLD_AT_TARGET;
            }
            break;

        case STEPPER_ARM_HOLD_AT_TARGET:
            if ((HAL_GetTick() - arm_hold_start_ms) >= ARM_DROPOFF_HOLD_MS)
            {
                arm_target_idx = 0U;
                delta = Arm_CalcDelta();

                if (delta != 0)
                {
                    Stepper_Arm_SetCommand(delta);
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

    // target_id 유효성 판단 및 slider_target_idx, arm_target_idx 계산
    if (Stepper_TargetIdToIdx(target_id) == 0U)
    {
        return 0U;
    }

    dropoff_target_id = target_id;
    dropoff_done_latched = 0U;
    arm_hold_start_ms = 0U;

    /*
     * 항상 arm을 먼저 center로 복귀시킨 뒤
     * slider 이동 → arm target 이동 → hold → arm center 복귀 순서로 수행한다.
     */
    arm_target_idx = 0U;
    delta = Arm_CalcDelta();

    if (delta != 0)
    {
        Stepper_Arm_SetCommand(delta);
        stepper_state = STEPPER_ARM_TO_INIT_PRE;
    }
    else
    {
        arm_curr_idx = 0U;

        delta = Slider_CalcDelta();
        if (delta != 0)
        {
            Stepper_Slider_SetCommand(delta);
        }

        stepper_state = STEPPER_SLIDER_TO_TARGET;
    }

    return 1U;
}

// homing 중이든, dropoff 중이든, arm post-init 중이든 다 busy로 봅니다.
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

void Stepper_SliderJogStart(int8_t dir)
{
    if (stepper_state != STEPPER_IDLE)
    {
        return;
    }

    if (slider_busy == 1U)
    {
        return;
    }

    if (dir > 0)
    {
        HAL_GPIO_WritePin(slider_DIR_GPIO_Port, slider_DIR_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(slider_DIR_GPIO_Port, slider_DIR_Pin, GPIO_PIN_RESET);
    }

    slider_steps_remain = 0U;
    slider_busy = 1U;
    stepper_state = STEPPER_SLIDER_MANUAL;

    __HAL_TIM_SET_COUNTER(&htim5, 0U);
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, SLIDER_PULSE_DUTY);

    HAL_TIM_Base_Start_IT(&htim5);
}

void Stepper_SliderJogStop(void)
{
    if (stepper_state != STEPPER_SLIDER_MANUAL)
    {
        return;
    }

    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, 0U);
    HAL_TIM_Base_Stop_IT(&htim5);

    slider_steps_remain = 0U;
    slider_busy = 0U;

    /*
     * 버튼 수동 후진은 slider를 물리적 기준 위치로 되돌리는 용도.
     * 버튼을 뗀 시점을 software index 0으로 간주한다.
     */
    slider_curr_idx = 0U;
    slider_target_idx = 0U;

    stepper_state = STEPPER_IDLE;
}

void Stepper_Slider_SetCommand(int32_t cmd)
{
    uint16_t steps = Stepper_AbsClampSteps(cmd);

    if ((cmd == 0) || (slider_busy == 1U))
    {
    	return;
    }

    if (cmd > 0)
    {
        HAL_GPIO_WritePin(slider_DIR_GPIO_Port, slider_DIR_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(slider_DIR_GPIO_Port, slider_DIR_Pin, GPIO_PIN_RESET);
    }

    slider_steps_remain = steps;
    slider_busy = 1U;

    __HAL_TIM_SET_COUNTER(&htim5, 0U);
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, SLIDER_PULSE_DUTY);

    HAL_TIM_Base_Start_IT(&htim5); // 인터럽트 시작
}

void Stepper_Arm_SetCommand(int32_t cmd)
{
    uint16_t steps = Stepper_AbsClampSteps(cmd);

    if ((cmd == 0) || (arm_busy == 1U))
    {
        return;
    }

    if (cmd > 0)
    {
        HAL_GPIO_WritePin(arm_DIR_GPIO_Port, arm_DIR_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(arm_DIR_GPIO_Port, arm_DIR_Pin, GPIO_PIN_RESET);
    }

    arm_steps_remain = steps;
    arm_busy = 1U;

    __HAL_TIM_SET_COUNTER(&htim8, 0U);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, ARM_PULSE_DUTY);

    HAL_TIM_Base_Start_IT(&htim8); // 인터럽트 시작
}

void Stepper_StopAll(void)
{
    __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, 0U);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0U);

    HAL_TIM_Base_Stop_IT(&htim5);
    HAL_TIM_Base_Stop_IT(&htim8);

    slider_steps_remain = 0U;
    arm_steps_remain    = 0U;

    slider_busy = 0U;
    arm_busy    = 0U;

    stepper_state = STEPPER_IDLE;

    dropoff_done_latched = 0U;
    dropoff_target_id    = 0U;

    arm_hold_start_ms = 0U;
    slider_hold_start_ms = 0U;
}

void Stepper_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM5) // slider
    {
        if (slider_busy == 1U)
        {
            if (stepper_state == STEPPER_SLIDER_MANUAL)
            {
                return;
            }

            if (slider_steps_remain > 0U)
            {
                slider_steps_remain--;
            }

            if (slider_steps_remain == 0U)
            {
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, 0U);
                HAL_TIM_Base_Stop_IT(&htim5);
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
                HAL_TIM_Base_Stop_IT(&htim8);
                arm_busy = 0U;
            }
        }
    }
}

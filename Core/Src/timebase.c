/*
 * timebase.c
 *
 *  Created on: Apr 2, 2026
 *      Author: kyubeom
 */

#include "timebase.h"
#include "tim.h"


/* =========================================
 * static 변수 및 함수
 * ========================================= */


/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. 타이머2 초기화
void Timebase_Init(void)
{
	HAL_TIM_Base_Start(&htim2);
}


/* =========================================
 * global 함수
 * ========================================= */
// 1. 타이머 us 값 (raw) getter (최대 71.6min, over시간에 대해 로직 만들어야함)
// 2. 타이머 ms 값 getter

uint32_t Timebase_GetMicros(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

uint32_t Timebase_GetMillis(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2) / 1000U;
}

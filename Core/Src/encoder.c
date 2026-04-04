/*
 * encoder.c
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#include "encoder.h"
#include "tim.h"
#include "config.h"

/* =========================================
 * static 변수 및 함수
 * ========================================= */
// 1. 엔코더 구조체 2개 선언 (enc_left , enc_right)
// 2. SAMPLE_TIME_S 기준 엔코더 카운트를 rpm으로 변환
// 3. RPM_LPF_ALPHA 기준 LPF 적용

static Encoder_t enc_left;
static Encoder_t enc_right;

static float Encoder_CalcRPM(int16_t delta)
{
    float counts_per_rev = ENCODER_CPR * GEAR_RATIO;
    return (delta / counts_per_rev) * (60.0f / SAMPLE_TIME_S);
}




/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. 엔코더 초기화 (반드시 Motor_Init(); 후에 적용)

void Encoder_Init(void)
{
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

	// LEFT
    enc_left.count_now  = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    enc_left.count_prev = enc_left.count_now;
    enc_left.delta      = 0;
    enc_left.total      = 0;
    enc_left.rpm        = 0.0f;

    // RIGHT
    enc_right.count_now  = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
    enc_right.count_prev = enc_right.count_now;
    enc_right.delta      = 0;
    enc_right.total      = 0;
    enc_right.rpm        = 0.0f;
}



/* =========================================
 * global 함수
 * ========================================= */
// 1. 엔코더 업데이트 (현재 RPM 계산)
// 2. 엔코더 리셋 함수
// 3. getter 함수

void Encoder_Update(void)
{
	// LEFT
    enc_left.count_now = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    enc_left.delta = (int16_t)(enc_left.count_now - enc_left.count_prev);
    enc_left.total += enc_left.delta;
    enc_left.rpm = Encoder_CalcRPM(enc_left.delta);
    enc_left.count_prev = enc_left.count_now;

    // RIGHT
    enc_right.count_now = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
    enc_right.delta = (int16_t)(enc_right.count_now - enc_right.count_prev);
    enc_right.total += enc_right.delta;
    enc_right.rpm = Encoder_CalcRPM(enc_right.delta);
    enc_right.count_prev = enc_right.count_now;
}

void Encoder_Reset(void)
{
	Encoder_Init();
}

int16_t Encoder_Left_GetNow(void)     { return enc_left.count_now;   }
int16_t Encoder_Left_GetPrev(void)    { return enc_left.count_prev;  }
int16_t Encoder_Left_GetDelta(void)   { return enc_left.delta; 		 }
int32_t Encoder_Left_GetTotal(void)   { return enc_left.total;		 }
float   Encoder_Left_GetRpm(void)     { return enc_left.rpm;   		 }

int16_t Encoder_Right_GetNow(void)    { return -enc_right.count_now; }
int16_t Encoder_Right_GetPrev(void)   { return -enc_right.count_prev;}
int16_t Encoder_Right_GetDelta(void)  { return -enc_right.delta;	 }
int32_t Encoder_Right_GetTotal(void)  { return -enc_right.total;	 }
float   Encoder_Right_GetRpm(void)    { return -enc_right.rpm;  	 }

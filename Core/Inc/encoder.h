/*
 * encoder.h
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

// 기능 설명:
// 엔코더를 입력으로 받아 현재 RPM 계산

#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_

#include "main.h"

typedef struct
{
    int16_t count_now;  // 현재 카운트
    int16_t count_prev; // 이전 카운트
    int16_t delta;		// 현재 카운트 - 이전 카운트
    int32_t total;      // 누적 카운트
    float rpm;          // 현재 rpm
} Encoder_t;

void Encoder_Init(void);

void Encoder_Update(void);
void Encoder_Reset(void);

int16_t Encoder_Left_GetNow(void);
int16_t Encoder_Left_GetPrev(void);
int16_t Encoder_Left_GetDelta(void);
int32_t Encoder_Left_GetTotal(void);
float   Encoder_Left_GetRpm(void);

int16_t Encoder_Right_GetNow(void);
int16_t Encoder_Right_GetPrev(void);
int16_t Encoder_Right_GetDelta(void);
int32_t Encoder_Right_GetTotal(void);
float   Encoder_Right_GetRpm(void);

#endif /* INC_ENCODER_H_ */

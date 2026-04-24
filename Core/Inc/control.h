/*
 * control.h
 *
 *  Created on: Apr 2, 2026
 *      Author: kyubeom
 */

#ifndef INC_CONTROL_H_
#define INC_CONTROL_H_

#include "main.h"

typedef struct
{
    float ref_rpm;
    float meas_rpm;
    float err;
    float integral;
    int16_t pwm_cmd;
} WheelPI_t;

typedef struct
{
	// 외부에서 들어온 최종 목표 명령
    float v_cmd_mps;
    float w_cmd_radps;

    // ramp 적용 후 실제 PI에 들어가는 명령
    float v_ref_mps;
    float w_ref_radps;

    WheelPI_t left;
    WheelPI_t right;
} Control_t;

void Control_Init(void);
void Control_SetTargetRPM(float left_rpm, float right_rpm);
void Control_SetTargetVW(float v_mps, float w_radps);
void Control_Update(void);
void Control_Stop(void);

float Control_GetLeftRefRPM(void);
float Control_GetLeftMeasRPM(void);
int16_t Control_GetLeftPwmCmd(void);

float Control_GetRightRefRPM(void);
float Control_GetRightMeasRPM(void);
int16_t Control_GetRightPwmCmd(void);

float Control_GetCmdV(void);
float Control_GetCmdW(void);
float Control_GetRefV(void);
float Control_GetRefW(void);

float Control_GetLeftIntegral(void);
float Control_GetRightIntegral(void);

#endif /* INC_CONTROL_H_ */

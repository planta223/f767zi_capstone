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

#endif /* INC_CONTROL_H_ */

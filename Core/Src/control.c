/*
 * control.c
 *
 *  Created on: Apr 2, 2026
 *      Author: kyubeom
 */


#include "control.h"
#include "encoder.h"
#include "motor.h"
#include "config.h"

/* =========================================
 * static 변수 및 함수
 * ========================================= */

static Control_t ctrl;

static float ClampFloat(float x, float min_val, float max_val)
{
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. control 초기화

void Control_Init(void)
{
    ctrl.v_ref_mps = 0.0f;
    ctrl.w_ref_radps = 0.0f;

    ctrl.left.ref_rpm   = 0.0f;
    ctrl.left.meas_rpm  = 0.0f;
    ctrl.left.err       = 0.0f;
    ctrl.left.integral  = 0.0f;
    ctrl.left.pwm_cmd   = 0;

    ctrl.right.ref_rpm  = 0.0f;
    ctrl.right.meas_rpm = 0.0f;
    ctrl.right.err      = 0.0f;
    ctrl.right.integral = 0.0f;
    ctrl.right.pwm_cmd  = 0;
}

/* =========================================
 * global 함수
 * ========================================= */
// 1. 목표 rpm 직접 설정
// 2. 목표 v,w 설정
// 3. control 업데이트
// 4. control 정지
// 5. getter 함수

void Control_SetTargetRPM(float left_rpm, float right_rpm)
{
    ctrl.left.ref_rpm = left_rpm;
    ctrl.right.ref_rpm = right_rpm;
}

void Control_SetTargetVW(float v_mps, float w_radps)
{
    float v_left;
    float v_right;

    v_mps   = ClampFloat(v_mps,   -CMD_V_MAX_MPS,   CMD_V_MAX_MPS);
    w_radps = ClampFloat(w_radps, -CMD_W_MAX_RADPS, CMD_W_MAX_RADPS);

    ctrl.v_ref_mps = v_mps;
    ctrl.w_ref_radps = w_radps;

    v_left  = v_mps - 0.5f * WHEEL_BASE_M * w_radps;
    v_right = v_mps + 0.5f * WHEEL_BASE_M * w_radps;

    // v_left, v_right 중 하나라도 모터가 감당가능한 속도를 넘을 시,
    // 이를 scaling하는 로직 필요

    ctrl.left.ref_rpm  = (v_left  / (2.0f * PI_F * WHEEL_RADIUS_M)) * 60.0f;
    ctrl.right.ref_rpm = (v_right / (2.0f * PI_F * WHEEL_RADIUS_M)) * 60.0f;
}


void Control_Update(void)
{
    float left_u;
    float right_u;

    // 현재 rpm 측정
    ctrl.left.meas_rpm  = Encoder_Left_GetRpm();
    ctrl.right.meas_rpm = Encoder_Right_GetRpm();

    // 현재 error 계산
    ctrl.left.err  = ctrl.left.ref_rpm  - ctrl.left.meas_rpm;
    ctrl.right.err = ctrl.right.ref_rpm - ctrl.right.meas_rpm;

    // 현재 오차 적분값 계산
    ctrl.left.integral  += ctrl.left.err  * CONTROL_TS_S;
    ctrl.right.integral += ctrl.right.err * CONTROL_TS_S;

    // anti-windup
    ctrl.left.integral  = ClampFloat(ctrl.left.integral,  -CTRL_I_LIMIT, CTRL_I_LIMIT);
    ctrl.right.integral = ClampFloat(ctrl.right.integral, -CTRL_I_LIMIT, CTRL_I_LIMIT);

    // PI 출력 계산
    left_u  = CTRL_KP * ctrl.left.err  + CTRL_KI * ctrl.left.integral;
    right_u = CTRL_KP * ctrl.right.err + CTRL_KI * ctrl.right.integral;

    // 출력 saturation
    if (left_u > MOTOR_PWM_MAX) left_u = MOTOR_PWM_MAX;
    if (left_u < -MOTOR_PWM_MAX) left_u = -MOTOR_PWM_MAX;

    if (right_u > MOTOR_PWM_MAX) right_u = MOTOR_PWM_MAX;
    if (right_u < -MOTOR_PWM_MAX) right_u = -MOTOR_PWM_MAX;

    // 최종 명령 저장
    ctrl.left.pwm_cmd  = (int16_t)left_u;
    ctrl.right.pwm_cmd = (int16_t)right_u;

    // 모터 출력
    Motor_Both_SetCommand(ctrl.left.pwm_cmd, ctrl.right.pwm_cmd);
}

void Control_Stop(void)
{
    ctrl.v_ref_mps   = 0.0f;
    ctrl.w_ref_radps = 0.0f;

    ctrl.left.ref_rpm   = 0.0f;
    ctrl.left.err       = 0.0f;
    ctrl.left.integral  = 0.0f;
    ctrl.left.pwm_cmd   = 0;

    ctrl.right.ref_rpm  = 0.0f;
    ctrl.right.err      = 0.0f;
    ctrl.right.integral = 0.0f;
    ctrl.right.pwm_cmd  = 0;

    Motor_StopAll();
}

float Control_GetLeftRefRPM(void)    { return ctrl.left.ref_rpm;   }
float Control_GetLeftMeasRPM(void)   { return ctrl.left.meas_rpm;  }
int16_t Control_GetLeftPwmCmd(void)  { return ctrl.left.pwm_cmd;   }

float Control_GetRightRefRPM(void)   { return ctrl.right.ref_rpm;  }
float Control_GetRightMeasRPM(void)  { return ctrl.right.meas_rpm; }
int16_t Control_GetRightPwmCmd(void) { return ctrl.right.pwm_cmd;  }

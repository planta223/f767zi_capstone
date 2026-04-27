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

static float RampFloat(float current, float target, float max_delta)
{
    float diff = target - current;

    if (diff > max_delta)
    {
        return current + max_delta;
    }
    else if (diff < -max_delta)
    {
        return current - max_delta;
    }
    else
    {
        return target;
    }
}

/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. control 초기화

void Control_Init(void)
{
    ctrl.v_cmd_mps = 0.0f;
    ctrl.w_cmd_radps = 0.0f;

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
    v_mps   = ClampFloat(v_mps,   -CMD_V_MAX_MPS,   CMD_V_MAX_MPS);
    w_radps = ClampFloat(w_radps, -CMD_W_MAX_RADPS, CMD_W_MAX_RADPS);

    ctrl.v_cmd_mps = v_mps;
    ctrl.w_cmd_radps = w_radps;
}


void Control_Update(void)
{
    float left_p;
    float right_p;

    float left_i_candidate;
    float right_i_candidate;

    float left_u_unsat;
    float right_u_unsat;

    float left_u_sat;
    float right_u_sat;

    float max_dv;
    float max_dw;

    float v_left;
    float v_right;

    /* 1. v,w command ramp */
    max_dv = CMD_V_ACCEL_MAX_MPS2 * CONTROL_TS_S;
    max_dw = CMD_W_ACCEL_MAX_RADPS2 * CONTROL_TS_S;

    ctrl.v_ref_mps = RampFloat(ctrl.v_ref_mps, ctrl.v_cmd_mps, max_dv);
    ctrl.w_ref_radps = RampFloat(ctrl.w_ref_radps, ctrl.w_cmd_radps, max_dw);

    /* 2. v,w -> wheel linear velocity */
    v_left  = ctrl.v_ref_mps - 0.5f * WHEEL_BASE_M * ctrl.w_ref_radps;
    v_right = ctrl.v_ref_mps + 0.5f * WHEEL_BASE_M * ctrl.w_ref_radps;

    /* 3. wheel linear velocity -> wheel rpm */
    ctrl.left.ref_rpm  = (v_left  / (2.0f * PI_F * WHEEL_RADIUS_M)) * 60.0f;
    ctrl.right.ref_rpm = (v_right / (2.0f * PI_F * WHEEL_RADIUS_M)) * 60.0f;

    /* 4. 정지 명령이면 integral 제거 */
    if ((ctrl.left.ref_rpm > -CTRL_REF_ZERO_EPS_RPM) &&
        (ctrl.left.ref_rpm <  CTRL_REF_ZERO_EPS_RPM))
    {
        ctrl.left.integral = 0.0f;
    }

    if ((ctrl.right.ref_rpm > -CTRL_REF_ZERO_EPS_RPM) &&
        (ctrl.right.ref_rpm <  CTRL_REF_ZERO_EPS_RPM))
    {
        ctrl.right.integral = 0.0f;
    }

    /* 5. 현재 rpm 측정 */
    ctrl.left.meas_rpm  = Encoder_Left_GetRpm();
    ctrl.right.meas_rpm = Encoder_Right_GetRpm();

    /* 6. 현재 error 계산 */
    ctrl.left.err  = ctrl.left.ref_rpm  - ctrl.left.meas_rpm;
    ctrl.right.err = ctrl.right.ref_rpm - ctrl.right.meas_rpm;

    /* 7. P항 계산 */
    left_p  = CTRL_KP * ctrl.left.err;
    right_p = CTRL_KP * ctrl.right.err;

    /* 8. 적분 후보값 계산 */
    left_i_candidate  = ctrl.left.integral  + ctrl.left.err  * CONTROL_TS_S;
    right_i_candidate = ctrl.right.integral + ctrl.right.err * CONTROL_TS_S;

    /* 9. 적분 후보값 제한 */
    left_i_candidate  = ClampFloat(left_i_candidate,  -CTRL_I_LIMIT, CTRL_I_LIMIT);
    right_i_candidate = ClampFloat(right_i_candidate, -CTRL_I_LIMIT, CTRL_I_LIMIT);

    /* 10. 적분 후보값 기준 출력 계산 */
    left_u_unsat  = left_p  + CTRL_KI * left_i_candidate;
    right_u_unsat = right_p + CTRL_KI * right_i_candidate;

    /* 11. 출력 saturation */
    left_u_sat  = ClampFloat(left_u_unsat,  -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
    right_u_sat = ClampFloat(right_u_unsat, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);

    /*
     * 12. Conditional integration anti-windup
     *
     * 출력이 포화되지 않았으면 적분 반영.
     * +포화 상태에서 error가 음수이면 포화를 줄이는 방향이므로 적분 반영.
     * -포화 상태에서 error가 양수이면 포화를 줄이는 방향이므로 적분 반영.
     */
    if ((left_u_unsat == left_u_sat) ||
        ((left_u_sat >=  MOTOR_PWM_MAX) && (ctrl.left.err < 0.0f)) ||
        ((left_u_sat <= -MOTOR_PWM_MAX) && (ctrl.left.err > 0.0f)))
    {
        ctrl.left.integral = left_i_candidate;
    }

    if ((right_u_unsat == right_u_sat) ||
        ((right_u_sat >=  MOTOR_PWM_MAX) && (ctrl.right.err < 0.0f)) ||
        ((right_u_sat <= -MOTOR_PWM_MAX) && (ctrl.right.err > 0.0f)))
    {
        ctrl.right.integral = right_i_candidate;
    }

    /* 13. 최종 출력 재계산 */
    left_u_unsat  = left_p  + CTRL_KI * ctrl.left.integral;
    right_u_unsat = right_p + CTRL_KI * ctrl.right.integral;

    left_u_sat  = ClampFloat(left_u_unsat,  -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
    right_u_sat = ClampFloat(right_u_unsat, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);

    /* 14. 최종 명령 저장 */
    ctrl.left.pwm_cmd  = (int16_t)left_u_sat;
    ctrl.right.pwm_cmd = (int16_t)right_u_sat;

    /* 15. 모터 출력 */
    Motor_Both_SetCommand(ctrl.left.pwm_cmd, ctrl.right.pwm_cmd);
}

void Control_Stop(void)
{
    ctrl.v_cmd_mps   = 0.0f;
    ctrl.w_cmd_radps = 0.0f;

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

float Control_GetCmdV(void) { return ctrl.v_cmd_mps; }
float Control_GetCmdW(void) { return ctrl.w_cmd_radps; }

float Control_GetRefV(void) { return ctrl.v_ref_mps; }
float Control_GetRefW(void) { return ctrl.w_ref_radps; }

float Control_GetLeftIntegral(void)  { return ctrl.left.integral; }
float Control_GetRightIntegral(void) { return ctrl.right.integral; }

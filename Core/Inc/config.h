/*
 * config.h
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

/* =========================================
 * motor.c
 * ========================================= */
#define MOTOR_PWM_MAX          10799     // TIM1 ARR = 10799

/* =========================================
 * encoder.c
 * ========================================= */
#define ENCODER_CPR            64.0f     // encoder CPR (x4 반영값)
#define GEAR_RATIO             50.0f     // gear ratio
#define SAMPLE_TIME_S          0.01f     // encoder update period = 10 ms

/* =========================================
 * protocol.c
 * ========================================= */
#define PROTOCOL_RX_BUF_SIZE   16U
#define PROTOCOL_CMD           2000.0f

/* =========================================
 * odometry.c
 * ========================================= */
#define ODOM_TS_S              0.01f     // odometry update period = 10 ms
#define WHEEL_RADIUS_M         0.05f     // wheel radius [m]
#define WHEEL_BASE_M           0.233f     // wheel center-to-center distance [m] (must measure)
#define PI_F                   3.14159265359f

/* =========================================
 * control.c
 * ========================================= */
#define CONTROL_TS_S          0.01f  // 제어 주기
#define CTRL_KP               50.0f  // P 계수
#define CTRL_KI               500.0f   // I 계수
#define CTRL_I_LIMIT          300.0f // Anti-windup 계수

#define CMD_V_MAX_MPS         0.50f  // 입력 선속도 제한
#define CMD_W_MAX_RADPS       3.00f  // 입력 각속도 제한

/* =========================================
 * stepper.c
 * ========================================= */
#define STEPPER_DIR_SETUP_US          10U

#define STEPPER_SLIDER_MIN_HZ         1.0f
#define STEPPER_SLIDER_MAX_HZ         3000.0f

#define STEPPER_ARM_MIN_HZ            1.0f
#define STEPPER_ARM_MAX_HZ            2000.0f

#define STEPPER_PWM_DUTY_NUM          1U
#define STEPPER_PWM_DUTY_DEN          2U


#endif /* INC_CONFIG_H_ */

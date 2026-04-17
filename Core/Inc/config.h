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
#define PROTOCOL_SOF1               0xAAU
#define PROTOCOL_SOF2               0x55U

#define MSG_TYPE_ODOM           	0x01U  // 송신
#define MSG_TYPE_VW             	0x02U  // 수신
#define MSG_TYPE_DROPOFF_START  	0x03U  // 수신
#define MSG_TYPE_DROPOFF_DONE   	0x04U  // 송신
#define MSG_TYPE_HEARTBEAT      	0x05U  // 수신

#define FRAME_SIZE_ODOM             28U
#define FRAME_SIZE_VW               12U
#define FRAME_SIZE_DROPOFF_START    5U
#define FRAME_SIZE_DROPOFF_DONE     4U
#define FRAME_SIZE_HEARTBEAT        4U

#define RX_FRAME_MAX_SIZE           12U
#define TIMEOUT_MS                  200U

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
#define CTRL_KI               500.0f // I 계수
#define CTRL_I_LIMIT          300.0f // Anti-windup 계수

#define CMD_V_MAX_MPS         0.50f  // 입력 선속도 제한
#define CMD_W_MAX_RADPS       3.00f  // 입력 각속도 제한

/* =========================================
 * stepper.c
 * ========================================= */
#define STEPPER_STEPS_MAX     1000   //
#define STEPPER_PULSE_DUTY    500

/* =========================================
 * failsafe.c
 * ========================================= */
#define HEARTBEAT_TIMEOUT_MS   500U

#endif /* INC_CONFIG_H_ */

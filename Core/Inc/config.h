/*
 * config.h
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

/* =========================================
 * main.c
 * ========================================= */
#define IWDG_REFRESH_PERIOD_MS		  100U // IWDG refresh 주기[ms]

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
#define WHEEL_BASE_M           0.700f     // wheel center-to-center distance [m] (must measure)
#define PI_F                   3.14159265359f

/* =========================================
 * control.c
 * ========================================= */
#define CONTROL_TS_S            0.01f  // 제어 주기
#define CTRL_KP                 50.0f  // P 계수
#define CTRL_KI                 500.0f // I 계수
#define CTRL_I_LIMIT            5.0f   // Anti-windup 계수

#define CTRL_REF_ZERO_EPS_RPM   0.5f

#define CMD_V_MAX_MPS           0.12f  // 입력 선속도 제한
#define CMD_W_MAX_RADPS         0.30f  // 입력 각속도 제한

#define CMD_V_ACCEL_MAX_MPS2        0.05f // 일단 보수적으로. 0.10으로 합의
#define CMD_W_ACCEL_MAX_RADPS2      0.15f // 일단 보수적으로. 0.25으로 합의

/* =========================================
 * stepper.c
 * ========================================= */

#define SLIDER_PULSE_DUTY     500U     // TIM5 ARR = 999, 50%
#define ARM_PULSE_DUTY        5000U    // TIM8 ARR = 9999, 50%

/* -----------------------------------------
 * Slider hardware
 * - Driver    : DM556
 * - Motor     : 57HS5630A4, 1.2 Nm, 3 A/phase
 * - Current   : 2.7 A
 * - Microstep : 400 pulse/rev
 * - Table     : SFU1605, 5 mm/rev lead, 600 mm stroke
 * ----------------------------------------- */
#define SLIDER_LEAD_MM_PER_REV          5U
#define SLIDER_DRIVER_PULSES_PER_REV    400U
#define SLIDER_STROKE_MM                600U

#define SLIDER_PULSES_PER_MM            \
    (SLIDER_DRIVER_PULSES_PER_REV / SLIDER_LEAD_MM_PER_REV)

#define SLIDER_STROKE_PULSES            \
    (SLIDER_STROKE_MM * SLIDER_PULSES_PER_MM)

/*
 * idx 1 = OFFSET
 * idx 2 = OFFSET + GAP
 * idx 3 = OFFSET + 2*GAP
 */
#define SLIDER_OFFSET_MM                0U
#define SLIDER_GAP_MM                   247U

#define SLIDER_OFFSET_PULSES            \
    (SLIDER_OFFSET_MM * SLIDER_PULSES_PER_MM)

#define SLIDER_GAP_PULSES               \
    (SLIDER_GAP_MM * SLIDER_PULSES_PER_MM)

#define SLIDER_MAX_TARGET_MM            \
    (SLIDER_OFFSET_MM + (2U * SLIDER_GAP_MM))

#define SLIDER_MAX_TARGET_PULSES        \
    (SLIDER_MAX_TARGET_MM * SLIDER_PULSES_PER_MM)

/*
 * Current target max:
 * 494 mm * 80 pulse/mm = 39520 pulse
 * 41000 pulse gives small safety margin.
 */
#define STEPPER_CMD_PULSES_MAX          41000U

/*
 * Initial homing test:
 * -2000 pulse / 80 pulse/mm = -25 mm
 */
#define SLIDER_HOMING_PULSES            (-2000)


/* -----------------------------------------
 * Arm hardware
 * - Driver    : DM556
 * - Motor     : 57HS1123A4, 3 Nm, 3 A/phase
 * - Current   : 3.2 A
 * - Microstep : 3200 pulse/rev
 * - Target    : 70 deg
 * ----------------------------------------- */
#define ARM_DRIVER_PULSES_PER_REV       3200U
#define ARM_SIDE_ANGLE_DEG              70U
#define ARM_DROPOFF_HOLD_MS             1000U

#define ARM_SIDE_PULSES                 \
    ((ARM_DRIVER_PULSES_PER_REV * ARM_SIDE_ANGLE_DEG) / 360U)

/* =========================================
 * failsafe.c
 * ========================================= */
#define HEARTBEAT_TIMEOUT_MS   500U

#endif /* INC_CONFIG_H_ */

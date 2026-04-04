/*
 * odometry.c
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */


#include "odometry.h"
#include "encoder.h"
#include "config.h"

#include <math.h>

/* =========================================
 * static 변수 및 함수
 * ========================================= */
// 1. 계산용 상수
// 2. odometry 구조체 선언
// 3. (-PI <= angle < PI) 정규화 함수

#define COUNTS_PER_WHEEL_REV   (ENCODER_CPR * GEAR_RATIO)
#define WHEEL_CIRCUMFERENCE_M  (2.0f * PI_F * WHEEL_RADIUS_M)
#define METER_PER_COUNT        (WHEEL_CIRCUMFERENCE_M / COUNTS_PER_WHEEL_REV)

static Odometry_t odom;

static float Odometry_NormalizeAngle(float angle)
{
    while (angle >= PI_F)
    {
        angle -= 2 * PI_F;
    }

    while (angle < -PI_F)
    {
        angle += 2 * PI_F;
    }

    return angle;
}



/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. odometry 초기화

void Odometry_Init(void)
{
    odom.x_m = 0.0f;
    odom.y_m = 0.0f;
    odom.yaw_rad = 0.0f;
    odom.v_mps = 0.0f;
    odom.w_radps = 0.0f;
}

/* =========================================
 * global 함수
 * ========================================= */
// 1. odometry 업데이트 (현재 odometry 계산)
// 2. odometry 리셋 함수
// 2. getter 함수

void Odometry_Update(void)
{
    float dL;
    float dR;
    float dC;
    float dTheta;
    float yaw_mid;

    dL = (float)Encoder_Left_GetDelta()  * METER_PER_COUNT;
    dR = (float)Encoder_Right_GetDelta() * METER_PER_COUNT;

    dC = 0.5f * (dL + dR);
    dTheta = (dR - dL) / WHEEL_BASE_M;

    yaw_mid = odom.yaw_rad + (0.5f * dTheta);

    /* ROS 기준
     * +x : forward
     * +y : left
     * CCW: positive
     */
    odom.x_m += dC * cosf(yaw_mid);
    odom.y_m += dC * sinf(yaw_mid);
    odom.yaw_rad += dTheta;

    odom.yaw_rad = Odometry_NormalizeAngle(odom.yaw_rad);

    odom.v_mps = dC / ODOM_TS_S;
    odom.w_radps = dTheta / ODOM_TS_S;
}

void Odometry_Reset(void)
{
	Odometry_Init();
}

float Odometry_GetX(void)    { return odom.x_m;     }
float Odometry_GetY(void)    { return odom.y_m;     }
float Odometry_GetYaw(void)  { return odom.yaw_rad; }
float Odometry_GetV(void)    { return odom.v_mps;   }
float Odometry_GetW(void)    { return odom.w_radps; }

/*
 * estop.c
 *
 *  Created on: May 6, 2026
 *      Author: kyubeom
 */

#include "estop.h"

#include "control.h"
#include "stepper.h"
#include "gpio.h"

/* =========================================
 * static 변수 및 함수
 * ========================================= */
static uint8_t estop_active = 0U;


/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
void EStop_Init(void)
{
    estop_active = 0U;
}


/* =========================================
 * global 함수
 * ========================================= */
void EStop_Trigger(void)
{
    estop_active = 1U;

    /*
     * Software E-STOP:
     * - DC motor command/ref/integral/PWM clear
     * - Stepper pulse timer stop
     * - Dropoff state abort
     */
    Control_Stop();
    Stepper_StopAll();
}

uint8_t EStop_IsActive(void)
{
    return estop_active;
}

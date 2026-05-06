/*
 * heartbeat.c
 *
 *  Created on: Apr 16, 2026
 *      Author: kyubeom
 */


#include "heartbeat.h"
#include "gpio.h"

#include "config.h"

#include "control.h"
#include "stepper.h"


/* =========================================
 * static 변수 및 함수
 * ========================================= */
/* last_heartbeat_ms  : 마지막 heartbeat 수신 시각
 * heartbeat_timeout  : heartbeat timeout 상태 플래그
 * stop_latched       : timeout 시 정지 동작 수행 여부 플래그
 *
 */
static uint32_t last_heartbeat_ms = 0U;
static uint8_t  heartbeat_timeout = 1U;
static uint8_t  stop_latched = 0U;


/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
void Heartbeat_Init(void)
{
    last_heartbeat_ms = 0U;
    heartbeat_timeout = 1U; // 초기 상태는 timeout 상태로 간주
    stop_latched = 0U;

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}


/* =========================================
 * global 함수
 * ========================================= */
void Heartbeat_Notify(uint32_t now_ms)
{
    last_heartbeat_ms = now_ms;
    heartbeat_timeout = 0U;
    stop_latched = 0U;
}

uint8_t Heartbeat_IsTimeout(void)
{
    return heartbeat_timeout;
}

void Heartbeat_Update(uint32_t now_ms)
{
	// 아직 heartbeat를 한 번도 받은 적 없으면 timeout
    if (last_heartbeat_ms == 0U)
    {
        heartbeat_timeout = 1U;
    }
    // 마지막 heartbeat 이후 제한 시간 초과하면 timeout
    else if ((now_ms - last_heartbeat_ms) > HEARTBEAT_TIMEOUT_MS)
    {
        heartbeat_timeout = 1U;
    }
    // 최근 heartbeat 수신되면 정상
    else
    {
        heartbeat_timeout = 0U;
    }

    if (heartbeat_timeout == 1U)
    {
    	// timeout인 경우 LD2[Blue] OFF
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

        if (stop_latched == 0U)
        {
            Control_Stop();
            Stepper_StopAll();

            stop_latched = 1U;
        }
    }
    else
    {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        stop_latched = 0U;
    }
}

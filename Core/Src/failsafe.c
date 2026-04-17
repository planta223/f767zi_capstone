/*
 * failsafe.c
 *
 *  Created on: Apr 16, 2026
 *      Author: kyubeom
 */


#include "config.h"

#include "control.h"
#include "stepper.h"
#include "failsafe.h"


/* =========================================
 * static 변수 및 함수
 * ========================================= */
/* last_heartbeat_ms  : 마지막 heartbeat 수신 시각
 * heartbeat_timeout  : heartbeat timeout 상태 플래그
 * stop_latched       : timeout 시 정지 동작 수행 여부 플래그
 */
static uint32_t last_heartbeat_ms = 0U;
static uint8_t  heartbeat_timeout = 1U;
static uint8_t  stop_latched = 0U;


/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
void Failsafe_Init(void)
{
    last_heartbeat_ms = 0U;
    heartbeat_timeout = 1U; // 초기 상태는 timeout 상태로 간주
    stop_latched = 0U;
}


/* =========================================
 * global 함수
 * ========================================= */
void Failsafe_NotifyHeartbeat(uint32_t now_ms)
{
    last_heartbeat_ms = now_ms;
    heartbeat_timeout = 0U;
    stop_latched = 0U;
}

uint8_t Failsafe_IsHeartbeatTimeout(void)
{
    return heartbeat_timeout;
}

void Failsafe_Update(uint32_t now_ms)
{
    if (last_heartbeat_ms == 0U)
    {
        heartbeat_timeout = 1U;
    }
    else if ((now_ms - last_heartbeat_ms) > HEARTBEAT_TIMEOUT_MS)
    {
        heartbeat_timeout = 1U;
    }
    else
    {
        heartbeat_timeout = 0U;
    }

    if (heartbeat_timeout == 1U)
    {
        if (stop_latched == 0U)
        {
            Control_Stop();
            Stepper_StopAll();
            stop_latched = 1U;
        }
    }
}

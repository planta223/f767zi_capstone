/*
 * protocol.c
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#include "protocol.h"
#include "usart.h"
#include "config.h"

#include "control.h"
#include "failsafe.h"
#include "stepper.h"

#include <string.h>

#define RX_STATE_WAIT_SOF1   0U
#define RX_STATE_WAIT_SOF2   1U
#define RX_STATE_COLLECT     2U

/* =========================================
 * TX static
 * ========================================= */

// UART3 실질 송신 (teleplot 전용)
static void Protocol_SendBuffer_debug(const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return;
    }

    HAL_UART_Transmit(&huart3, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

// 문자형 데이터 송신 (teleplot 전용)
static void Protocol_SendChar_debug(char ch)
{
    Protocol_SendBuffer_debug((const uint8_t *)&ch, 1U);
}

// 문자열형 데이터 송신 (teleplot 전용)
static void Protocol_SendString_debug(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    Protocol_SendBuffer_debug((const uint8_t *)str, (uint16_t)strlen(str));
}

// float를 간단히 ASCII로 전송 (teleplot 전용)
static void Protocol_SendFloatSimple_debug(float value, int decimals)
{
    int32_t iPart;
    float frac;

    if (decimals < 0)
    {
        decimals = 0;
    }

    if (value < 0.0f)
    {
        Protocol_SendChar_debug('-');
        value = -value;
    }

    iPart = (int32_t)value;
    frac  = value - (float)iPart;

    /* 정수부 출력 */
    {
        char tmpBuf[12];
        int idx = 0;

        if (iPart == 0)
        {
            tmpBuf[idx++] = '0';
        }
        else
        {
            while ((iPart > 0) && (idx < (int)(sizeof(tmpBuf) - 1)))
            {
                tmpBuf[idx++] = (char)('0' + (iPart % 10));
                iPart /= 10;
            }
        }

        while (idx > 0)
        {
            Protocol_SendChar_debug(tmpBuf[--idx]);
        }
    }

    /* 소수부 출력 */
    if (decimals > 0)
    {
        int i;

        Protocol_SendChar_debug('.');

        for (i = 0; i < decimals; i++)
        {
            int digit;

            frac *= 10.0f;
            digit = (int)frac;

            if (digit < 0)
            {
                digit = 0;
            }
            else if (digit > 9)
            {
                digit = 9;
            }

            Protocol_SendChar_debug((char)('0' + digit));
            frac -= (float)digit;
        }
    }
}


// uint32_t를 little-endian 4바이트로 버퍼에 기록 (odometry 전용)
static void Protocol_WriteU32LE(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)((value >> 0)  & 0xFFU);
    buf[1] = (uint8_t)((value >> 8)  & 0xFFU);
    buf[2] = (uint8_t)((value >> 16) & 0xFFU);
    buf[3] = (uint8_t)((value >> 24) & 0xFFU);
}

// float를 little-endian 4바이트로 버퍼에 기록 (odometry 전용)
static void Protocol_WriteFloatLE(uint8_t *buf, float value)
{
    union
    {
        float f;
        uint32_t u32;
    } conv;

    conv.f = value;
    Protocol_WriteU32LE(buf, conv.u32);
}

// checksum 1바이트 계산 (odometry 전용)
static uint8_t Checksum(const uint8_t *buf, uint16_t len)
{
    uint8_t sum = 0U;
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        sum ^= buf[i];
    }

    return sum;
}


/* =========================================
 * RX static
 * ========================================= */
/*
    rx_byte           : UART 인터럽트로 한 바이트 받을 임시 저장소
	rx_frame[]        : 조립 중인 수신 프레임 버퍼
	rx_state          : SOF1 대기 / SOF2 대기 / payload 수집 상태
	rx_index          : 현재 몇 바이트까지 모았는지
	rx_expected_len   : msg_type에 따른 프레임 길이
	rx_frame_ready    : 한 프레임 완성되었음을 main loop에 알리는 플래그
*/
static uint8_t rx_byte;
static uint8_t rx_frame[RX_FRAME_MAX_SIZE];
static uint8_t rx_state = RX_STATE_WAIT_SOF1;
static volatile uint16_t rx_index = 0U;
static volatile uint16_t rx_expected_len = 0U;
static volatile uint8_t rx_frame_ready = 0U;

// little endian 4바이트를 float로 계산 (vw_command 전용)
static float Protocol_ReadFloatLE(const uint8_t *buf)
{
    union
    {
        uint32_t u32;
        float f;
    } conv;

    conv.u32 =  ((uint32_t)buf[0] << 0)
              | ((uint32_t)buf[1] << 8)
              | ((uint32_t)buf[2] << 16)
              | ((uint32_t)buf[3] << 24);

    return conv.f;
}

// msg_type에 따른 프레임 길이 결정
static uint16_t Protocol_GetRxFrameSizeByType(uint8_t msg_type)
{
    switch (msg_type)
    {
        case MSG_TYPE_VW:
            return FRAME_SIZE_VW;

        case MSG_TYPE_DROPOFF_START:
            return FRAME_SIZE_DROPOFF_START;

        case MSG_TYPE_HEARTBEAT:
            return FRAME_SIZE_HEARTBEAT;

        default:
            return 0U;    // unknown
    }
}


/* =========================================
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
void Protocol_Init(void)
{
    rx_state = RX_STATE_WAIT_SOF1;
    rx_index = 0U;
    rx_expected_len = 0U;
    rx_frame_ready = 0U;
    memset(rx_frame, 0, sizeof(rx_frame));

    HAL_UART_Receive_IT(&huart3, &rx_byte, 1U);
}

/* =========================
 * TX global
 * ========================= */

/* Teleplot 전송 함수 */
/* 형식: [>][label][:][value][\n] */
void Protocol_SendTeleplot_debug(const char *label, float value, int decimals)
{
    if (label == NULL)
    {
        return;
    }

    Protocol_SendChar_debug('>');
    Protocol_SendString_debug(label);
    Protocol_SendChar_debug(':');
    Protocol_SendFloatSimple_debug(value, decimals);
    Protocol_SendChar_debug('\n');
}

/* Odometry 전송 함수 */
/* 형식: [SOF1][SOF2][MSG_TYPE][t_us][x_m][y_m][yaw_rad][v_mps][w_radps][checksum] */
void Protocol_SendOdometry(uint32_t t_us,
                           float x_m,
                           float y_m,
                           float yaw_rad,
                           float v_mps,
                           float w_radps)
{
    uint8_t frame[FRAME_SIZE_ODOM];

    frame[0] = PROTOCOL_SOF1;
    frame[1] = PROTOCOL_SOF2;
    frame[2] = MSG_TYPE_ODOM;

    Protocol_WriteU32LE(&frame[3],  t_us);
    Protocol_WriteFloatLE(&frame[7],  x_m);
    Protocol_WriteFloatLE(&frame[11],  y_m);
    Protocol_WriteFloatLE(&frame[15], yaw_rad);
    Protocol_WriteFloatLE(&frame[19], v_mps);
    Protocol_WriteFloatLE(&frame[23], w_radps);

    frame[FRAME_SIZE_ODOM - 1U] =
    		Checksum(frame, FRAME_SIZE_ODOM - 1U);

    HAL_UART_Transmit(&huart3, frame, FRAME_SIZE_ODOM, 10);
}

void Protocol_SendDropoffDone(void)
{
    uint8_t frame[FRAME_SIZE_DROPOFF_DONE];

    frame[0] = PROTOCOL_SOF1;
    frame[1] = PROTOCOL_SOF2;
    frame[2] = MSG_TYPE_DROPOFF_DONE;

    frame[FRAME_SIZE_DROPOFF_DONE - 1U] =
    		Checksum(frame, FRAME_SIZE_DROPOFF_DONE - 1U);

    HAL_UART_Transmit(&huart3, frame, FRAME_SIZE_DROPOFF_DONE, 10);
}

/* =========================
 * RX global
 * ========================= */

void Protocol_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART3)
    {
        return;
    }

    switch (rx_state)
    {
        case RX_STATE_WAIT_SOF1:
            if (rx_byte == PROTOCOL_SOF1)
            {
                rx_frame[0] = rx_byte;
                rx_index = 1U;
                rx_expected_len = 0U;
                rx_state = RX_STATE_WAIT_SOF2;
            }
            break;

        case RX_STATE_WAIT_SOF2:
            if (rx_byte == PROTOCOL_SOF2)
            {
                rx_frame[1] = rx_byte;
                rx_index = 2U;
                rx_state = RX_STATE_COLLECT;
            }
            else if (rx_byte == PROTOCOL_SOF1)
            {
                rx_frame[0] = PROTOCOL_SOF1;
                rx_index = 1U;
                rx_expected_len = 0U;
                rx_state = RX_STATE_WAIT_SOF2;
            }
            else
            {
                rx_index = 0U;
                rx_expected_len = 0U;
                rx_state = RX_STATE_WAIT_SOF1;
            }
            break;

        case RX_STATE_COLLECT:
            if (rx_index < RX_FRAME_MAX_SIZE)
            {
                rx_frame[rx_index++] = rx_byte;
            }
            else
            {
                rx_index = 0U;
                rx_expected_len = 0U;
                rx_state = RX_STATE_WAIT_SOF1;
                break;
            }

            /* msg_type를 막 받은 시점: rx_frame[2] */
            if (rx_index == 3U)
            {
                rx_expected_len = Protocol_GetRxFrameSizeByType(rx_frame[2]);

                if ((rx_expected_len == 0U) ||
                    (rx_expected_len > RX_FRAME_MAX_SIZE))
                {
                    rx_index = 0U;
                    rx_expected_len = 0U;
                    rx_state = RX_STATE_WAIT_SOF1;
                    break;
                }
            }

            if ((rx_expected_len > 0U) && (rx_index >= rx_expected_len))
            {
                rx_frame_ready = 1U;
                rx_index = 0U;
                rx_expected_len = 0U;
                rx_state = RX_STATE_WAIT_SOF1;
            }
            break;

        default:
            rx_index = 0U;
            rx_expected_len = 0U;
            rx_state = RX_STATE_WAIT_SOF1;
            break;
    }

    HAL_UART_Receive_IT(&huart3, &rx_byte, 1U);
}

/* 주행명령 수신 함수 (모터 구동) */
void Protocol_Process(void)
{
    uint8_t calc_xor;   // 수신 프레임 데이터로부터 계산한 checksum 값
    uint8_t recv_xor;   // 수신 프레임 마지막 바이트에 포함된 checksum 값
    uint8_t msg_type;   // 수신 프레임의 메시지 타입
    uint16_t frame_len; // msg_type에 따라 결정된 현재 프레임 전체 길이
    float v_mps;
    float w_radps;
    uint8_t target_id;
    /* 변수 추가 가능 */

    if (rx_frame_ready == 0U)
    {
        return;
    }

    rx_frame_ready = 0U;

    msg_type = rx_frame[2];
    frame_len = Protocol_GetRxFrameSizeByType(msg_type);

    if (frame_len == 0U)
    {
        return;
    }

    calc_xor = Checksum(rx_frame, frame_len - 1U);
    recv_xor = rx_frame[frame_len - 1U];

    if (calc_xor != recv_xor)
    {
        return;
    }

    switch (msg_type)
    {
        case MSG_TYPE_VW:
            if (Failsafe_IsHeartbeatTimeout() == 1U)
            {
                break;   // timeout 상태에서는 주행명령 무시
            }

            if (Stepper_IsBusy() == 1U)
            {
                break; // stepper busy 일때는 주행명령 무시
            }

            v_mps   = Protocol_ReadFloatLE(&rx_frame[3]);
            w_radps = Protocol_ReadFloatLE(&rx_frame[7]);
            Control_SetTargetVW(v_mps, w_radps);
            break;

        case MSG_TYPE_DROPOFF_START:
            if (Failsafe_IsHeartbeatTimeout() == 1U)
            {
                break;   // timeout 상태에서는 dropoff 무시
            }

        	target_id = rx_frame[3];

            /*
             * Dropoff 명령을 받은 순간 주행은 정지.
             * Dropoff 시작 실패 여부와 무관하게 정지 상태를 유지한다.
             */
            Control_Stop();

			if (Stepper_Dropoff_Start(target_id) == 0U)
			{
		        /*
		         * 현재는 실패 응답 frame이 없으므로 추가 동작 없음.
		         * 실패 원인 예:
		         * - stepper busy
		         * - invalid target_id
		         * - homing 미완료 상태
		         */
			}

        	break;

        case MSG_TYPE_HEARTBEAT:
        	Failsafe_NotifyHeartbeat(HAL_GetTick());
            break;

        default:
            break;
    }
}

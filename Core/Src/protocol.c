/*
 * protocol.c
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#include "protocol.h"
#include "usart.h"
#include "motor.h"
#include "config.h"
#include "control.h"

#include <string.h>
#include <stdio.h>




#define PROTOCOL_RX_MSG_TYPE_CMD_VW   0x02U
#define PROTOCOL_RX_FRAME_SIZE        12U

#define RX_STATE_WAIT_SOF1            0U
#define RX_STATE_WAIT_SOF2            1U
#define RX_STATE_COLLECT              2U



/* =========================================
 * static 변수 및 함수
 * ========================================= */
static uint8_t rx_byte;
static char rx_line_buf[PROTOCOL_RX_BUF_SIZE];
static volatile uint16_t rx_index = 0U;
static volatile uint8_t rx_ready = 0U;

static uint8_t rx_frame[PROTOCOL_RX_FRAME_SIZE];
static uint8_t rx_state = RX_STATE_WAIT_SOF1;
static volatile uint8_t rx_frame_ready = 0U;

static void Protocol_StartReceiveIT(void)
{
	// USART3로부터 1바이트 수신이 완료되면,
	// 인터럽트를 발생시키고,
	// 그 데이터를 rx_byte에 저장해라.
    HAL_UART_Receive_IT(&huart3, &rx_byte, 1U);
}

static void Protocol_ParseLine(const char *line);

static void Protocol_SendBuffer(const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return;
    }

    HAL_UART_Transmit(&huart3, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

static void Protocol_WriteU32LE(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)((value >> 0)  & 0xFFU);
    buf[1] = (uint8_t)((value >> 8)  & 0xFFU);
    buf[2] = (uint8_t)((value >> 16) & 0xFFU);
    buf[3] = (uint8_t)((value >> 24) & 0xFFU);
}

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

static void Protocol_SendByte(uint8_t byte)
{
    HAL_UART_Transmit(&huart3, &byte, 1U, HAL_MAX_DELAY);
}



// CRC (1바이트 xor)
static uint8_t Protocol_Checksum8(const uint8_t *buf, uint16_t len)
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
 * 모든 x.c 파일에는 X_Init(); 함수가 존재해야 한다.
 * ========================================= */
// 1. 수신 준비
/*
void Protocol_Init(void)
{
    rx_index = 0U;
    rx_ready = 0U;
    memset(rx_line_buf, 0, sizeof(rx_line_buf)); // 버퍼 값 전부 0으로 초기화 (안전 로직)

    Protocol_StartReceiveIT();
}
*/
void Protocol_Init(void)
{
    rx_state = RX_STATE_WAIT_SOF1;
    rx_index = 0U;
    rx_frame_ready = 0U;
    memset(rx_frame, 0, sizeof(rx_frame));

    Protocol_StartReceiveIT();
}

/* =========================
 * TX
 * ========================= */
// 문자형 데이터 송신
void Protocol_SendChar(char ch)
{
    Protocol_SendBuffer((const uint8_t *)&ch, 1U);
}

// 문자열형 데이터 송신
void Protocol_SendString(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    Protocol_SendBuffer((const uint8_t *)str, (uint16_t)strlen(str));
}

/* float를 간단히 ASCII로 전송 */
void Protocol_SendFloatSimple(float value, int decimals)
{
    int32_t iPart;
    float frac;

    if (decimals < 0)
    {
        decimals = 0;
    }

    if (value < 0.0f)
    {
        Protocol_SendChar('-');
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
            Protocol_SendChar(tmpBuf[--idx]);
        }
    }

    /* 소수부 출력 */
    if (decimals > 0)
    {
        int i;

        Protocol_SendChar('.');

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

            Protocol_SendChar((char)('0' + digit));
            frac -= (float)digit;
        }
    }
}


/* Teleplot 형식: >label:value */
void Protocol_SendTeleplot(const char *label, float value, int decimals)
{
    if (label == NULL)
    {
        return;
    }

    Protocol_SendChar('>');
    Protocol_SendString(label);
    Protocol_SendChar(':');
    Protocol_SendFloatSimple(value, decimals);
    Protocol_SendChar('\n');
}


void Protocol_SendOdometryBinary(uint32_t t_us,
                                 float x_m,
                                 float y_m,
                                 float yaw_rad,
                                 float v_mps,
                                 float w_radps)
{
    uint8_t frame[28];

    frame[0] = 0xAA;
    frame[1] = 0x55;
    frame[2] = 0x01;

    Protocol_WriteU32LE(&frame[3],  t_us);
    Protocol_WriteFloatLE(&frame[7],  x_m);
    Protocol_WriteFloatLE(&frame[11],  y_m);
    Protocol_WriteFloatLE(&frame[15], yaw_rad);
    Protocol_WriteFloatLE(&frame[19], v_mps);
    Protocol_WriteFloatLE(&frame[23], w_radps);

    frame[27] = Protocol_Checksum8(frame, 27U);

    HAL_UART_Transmit(&huart3, frame, 28U, 10);
}

/* =========================
 * RX
 * ========================= */
/*
void Protocol_RxCallback(UART_HandleTypeDef *huart)
{
    char ch;

    if (huart->Instance != USART3)
    {
        return;
    }

    // 방금 수신한 1바이트를 문자로 해석합니다.
    ch = (char)rx_byte;

    if (rx_ready == 0U)
    {
        if (ch == '\r') 	 // Carriage Return(\r)은 무시
        {
        	// ignore
        }
        else if (ch == '\n') // \n은 한줄 끝
        {
            rx_line_buf[rx_index] = '\0';
            rx_ready = 1U; // 한 줄 완성 플래그 on (available)
            rx_index = 0U; // 다음 줄을 위해 수신 인덱스 초기화
        }
        else 				 // else는 일반 문자 수신
        {
            if (rx_index < (PROTOCOL_RX_BUF_SIZE - 1U))
            {
                rx_line_buf[rx_index++] = ch;
            }
            else
            {
                // overflow: 현재 줄 폐기
            	// 동작은 현재 줄 폐기. 그러나 별로임. 로직 수정 필요.
                rx_index = 0U;
            }
        }
    }

    // error handling 필요할수도. 수정 필요.
    Protocol_StartReceiveIT();
}
*/

void Protocol_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART3)
    {
        return;
    }

    switch (rx_state)
    {
        case RX_STATE_WAIT_SOF1:
            if (rx_byte == 0xAAU)
            {
                rx_frame[0] = rx_byte;
                rx_state = RX_STATE_WAIT_SOF2;
            }
            break;

        case RX_STATE_WAIT_SOF2:
            if (rx_byte == 0x55U)
            {
                rx_frame[1] = rx_byte;
                rx_index = 2U;
                rx_state = RX_STATE_COLLECT;
            }
            else if (rx_byte == 0xAAU)
            {
                rx_frame[0] = 0xAAU;
                rx_state = RX_STATE_WAIT_SOF2;
            }
            else
            {
                rx_state = RX_STATE_WAIT_SOF1;
            }
            break;

        case RX_STATE_COLLECT:
            rx_frame[rx_index++] = rx_byte;

            if (rx_index >= PROTOCOL_RX_FRAME_SIZE)
            {
                rx_frame_ready = 1U;
                rx_state = RX_STATE_WAIT_SOF1;
                rx_index = 0U;
            }
            break;

        default:
            rx_state = RX_STATE_WAIT_SOF1;
            rx_index = 0U;
            break;
    }

    Protocol_StartReceiveIT();
}

/* =========================
 * RX Process
 * ========================= */
/*
void Protocol_Process(void)
{
    char line[PROTOCOL_RX_BUF_SIZE];

    if (rx_ready == 0U)
    {
        return;
    }

    strncpy(line, rx_line_buf, sizeof(line) - 1U);
    line[sizeof(line) - 1U] = '\0';
    rx_ready = 0U;

    Protocol_ParseLine(line);
}
*/

void Protocol_Process(void)
{
    uint8_t calc_xor;
    uint8_t recv_xor;
    uint8_t msg_type;
    float v_mps;
    float w_radps;

    if (rx_frame_ready == 0U)
    {
        return;
    }

    rx_frame_ready = 0U;

    msg_type = rx_frame[2];
    calc_xor = Protocol_Checksum8(rx_frame, PROTOCOL_RX_FRAME_SIZE - 1U);
    recv_xor = rx_frame[PROTOCOL_RX_FRAME_SIZE - 1U];

    if (calc_xor != recv_xor)
    {
        return;
    }

    if (msg_type != PROTOCOL_RX_MSG_TYPE_CMD_VW)
    {
        return;
    }

    v_mps   = Protocol_ReadFloatLE(&rx_frame[3]);
    w_radps = Protocol_ReadFloatLE(&rx_frame[7]);

    Control_SetTargetVW(v_mps, w_radps);
}

static void Protocol_ParseLine(const char *line)
{
    if (line == NULL || line[0] == '\0')
    {
        return;
    }

    switch (line[0])
    {
        case '8':   /* Forward */
        	Motor_Both_SetCommand(PROTOCOL_CMD, PROTOCOL_CMD);
            break;

        case '2':   /* Backward */
        	Motor_Both_SetCommand(-PROTOCOL_CMD, -PROTOCOL_CMD);
            break;

        case '4':   /* L eft turn */
        	Motor_Both_SetCommand(-PROTOCOL_CMD, PROTOCOL_CMD);
            break;

        case '6':   /* Right turn */
        	Motor_Both_SetCommand(PROTOCOL_CMD, -PROTOCOL_CMD);
            break;

        case '5':   /* Stop */
        	Motor_StopAll();
        	break;

        default:
            // do nothing
            break;
    }
}

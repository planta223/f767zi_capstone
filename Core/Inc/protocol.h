/*
 * protocol.h
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#ifndef INC_PROTOCOL_H_
#define INC_PROTOCOL_H_

#include "main.h"

void Protocol_Init(void);

void Protocol_SendChar(char ch);
void Protocol_SendString(const char *str);
void Protocol_SendFloatSimple(float value, int decimals);
void Protocol_SendTeleplot(const char *label, float value, int decimals);


void Protocol_SendOdometryBinary(uint32_t t_us,
                                 float x_m,
                                 float y_m,
                                 float yaw_rad,
                                 float v_mps,
                                 float w_radps);


void Protocol_RxCallback(UART_HandleTypeDef *huart);
void Protocol_Process(void);


#endif /* INC_PROTOCOL_H_ */

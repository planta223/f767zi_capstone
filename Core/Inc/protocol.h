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

void Protocol_SendTeleplot_debug(const char *label, float value, int decimals);
void Protocol_SendOdometry(uint32_t t_us,
                           float x_m,
                           float y_m,
                           float yaw_rad,
                           float v_mps,
                           float w_radps);
void Protocol_SendDropoffDone(void);


void Protocol_RxCallback(UART_HandleTypeDef *huart);
void Protocol_Process(void);


#endif /* INC_PROTOCOL_H_ */

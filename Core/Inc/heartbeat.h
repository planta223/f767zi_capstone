/*
 * heartbeat.h
 *
 *  Created on: Apr 16, 2026
 *      Author: kyubeom
 */

#ifndef INC_HEARTBEAT_H_
#define INC_HEARTBEAT_H_

#include <stdint.h>

void Heartbeat_Init(void);

void Heartbeat_Notify(uint32_t now_ms);
void Heartbeat_Update(uint32_t now_ms);

uint8_t Heartbeat_IsTimeout(void);


#endif /* INC_HEARTBEAT_H_ */

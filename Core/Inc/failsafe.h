/*
 * failsafe.h
 *
 *  Created on: Apr 16, 2026
 *      Author: kyubeom
 */

#ifndef INC_FAILSAFE_H_
#define INC_FAILSAFE_H_

#include <stdint.h>

void Failsafe_Init(void);

void Failsafe_NotifyHeartbeat(uint32_t now_ms);
void Failsafe_Update(uint32_t now_ms);

uint8_t Failsafe_IsHeartbeatTimeout(void);


#endif /* INC_FAILSAFE_H_ */

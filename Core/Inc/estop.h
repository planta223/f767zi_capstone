/*
 * estop.h
 *
 *  Created on: May 6, 2026
 *      Author: kyubeom
 */

#ifndef INC_ESTOP_H_
#define INC_ESTOP_H_

#include <stdint.h>

void EStop_Init(void);
void EStop_Trigger(void);
uint8_t EStop_IsActive(void);

#endif /* INC_ESTOP_H_ */

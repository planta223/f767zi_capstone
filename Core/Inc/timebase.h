/*
 * timebase.h
 *
 *  Created on: Apr 2, 2026
 *      Author: kyubeom
 */

#ifndef INC_TIMEBASE_H_
#define INC_TIMEBASE_H_

#include <stdint.h>

void Timebase_Init(void);

uint32_t Timebase_GetMicros(void);
uint32_t Timebase_GetMillis(void);

#endif /* INC_TIMEBASE_H_ */

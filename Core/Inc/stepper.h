/*
 * stepper.h
 *
 *  Created on: Apr 14, 2026
 *      Author: kyubeom
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "main.h"

void Stepper_Init(void);
void Stepper_Slider_SetCommand(int16_t cmd);
void Stepper_Arm_SetCommand(int16_t cmd);
void Stepper_StopAll(void);


#endif /* INC_STEPPER_H_ */

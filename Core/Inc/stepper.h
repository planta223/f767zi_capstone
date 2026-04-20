/*
 * stepper.h
 *
 *  Created on: Apr 14, 2026
 *      Author: kyubeom
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "main.h"

// startup을 2 단계 (arm, slider) 로 수정하면 더 깔끔할 듯
typedef enum
{
    STEPPER_IDLE = 0,
    STEPPER_STARTUP,

    STEPPER_ARM_TO_INIT_PRE,
    STEPPER_SLIDER_TO_TARGET,
    STEPPER_ARM_TO_TARGET,
    STEPPER_ARM_TO_INIT_POST,

    STEPPER_DONE
} StepperState_t;

void Stepper_Init(void);
void Stepper_Update(void);
uint8_t Stepper_Dropoff_Start(uint8_t target_id);
uint8_t Stepper_IsBusy(void);
uint8_t Stepper_GetAndClearDropoffDone(void);

void Stepper_Slider_SetCommand(int16_t cmd);
void Stepper_Arm_SetCommand(int16_t cmd);
void Stepper_StopAll(void);

void Stepper_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif /* INC_STEPPER_H_ */

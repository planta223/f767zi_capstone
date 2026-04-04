/*
 * motor.h
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

// 기능 설명:
// cmd를 입력으로 받아 모터 PWM을 직접 출력

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "main.h"

void Motor_Init(void);

void Motor_Left_SetCommand(int16_t cmd);
void Motor_Right_SetCommand(int16_t cmd);
void Motor_Both_SetCommand(int16_t left_cmd, int16_t right_cmd);
void Motor_StopAll(void);

#endif /* INC_MOTOR_H_ */

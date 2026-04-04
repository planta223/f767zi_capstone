/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_Btn_Pin GPIO_PIN_13
#define USER_Btn_GPIO_Port GPIOC
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define batt_VOLT_Pin GPIO_PIN_0
#define batt_VOLT_GPIO_Port GPIOC
#define slider_DIR_Pin GPIO_PIN_3
#define slider_DIR_GPIO_Port GPIOC
#define slider_PUL_Pin GPIO_PIN_3
#define slider_PUL_GPIO_Port GPIOA
#define imu_CS_Pin GPIO_PIN_4
#define imu_CS_GPIO_Port GPIOA
#define left_HA_Pin GPIO_PIN_6
#define left_HA_GPIO_Port GPIOA
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOB
#define left_PWM_Pin GPIO_PIN_9
#define left_PWM_GPIO_Port GPIOE
#define right_PWM_Pin GPIO_PIN_11
#define right_PWM_GPIO_Port GPIOE
#define LD3_Pin GPIO_PIN_14
#define LD3_GPIO_Port GPIOB
#define STLK_RX_Pin GPIO_PIN_8
#define STLK_RX_GPIO_Port GPIOD
#define STLK_TX_Pin GPIO_PIN_9
#define STLK_TX_GPIO_Port GPIOD
#define right_HA_Pin GPIO_PIN_12
#define right_HA_GPIO_Port GPIOD
#define right_HB_Pin GPIO_PIN_13
#define right_HB_GPIO_Port GPIOD
#define left_HB_Pin GPIO_PIN_7
#define left_HB_GPIO_Port GPIOC
#define arm_DIR_Pin GPIO_PIN_8
#define arm_DIR_GPIO_Port GPIOC
#define arm_PUL_Pin GPIO_PIN_9
#define arm_PUL_GPIO_Port GPIOC
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define right_DIR_Pin GPIO_PIN_9
#define right_DIR_GPIO_Port GPIOG
#define left_DIR_Pin GPIO_PIN_14
#define left_DIR_GPIO_Port GPIOG
#define imu_SCK_Pin GPIO_PIN_3
#define imu_SCK_GPIO_Port GPIOB
#define imu_MISO_Pin GPIO_PIN_4
#define imu_MISO_GPIO_Port GPIOB
#define imu_MOSI_Pin GPIO_PIN_5
#define imu_MOSI_GPIO_Port GPIOB
#define LD2_Pin GPIO_PIN_7
#define LD2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

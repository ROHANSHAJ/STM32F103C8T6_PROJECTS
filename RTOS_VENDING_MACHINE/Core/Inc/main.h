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
#include "stm32f1xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TRIG_Pin GPIO_PIN_0
#define TRIG_GPIO_Port GPIOB
#define ECHO_Pin GPIO_PIN_1
#define ECHO_GPIO_Port GPIOB
#define MB1_Pin GPIO_PIN_10
#define MB1_GPIO_Port GPIOB
#define MB2_Pin GPIO_PIN_11
#define MB2_GPIO_Port GPIOB
#define IR_Pin GPIO_PIN_12
#define IR_GPIO_Port GPIOB
#define IR_EXTI_IRQn EXTI15_10_IRQn
#define BUZZER_Pin GPIO_PIN_13
#define BUZZER_GPIO_Port GPIOB
#define PWM_Pin GPIO_PIN_8
#define PWM_GPIO_Port GPIOA
#define MC1_Pin GPIO_PIN_11
#define MC1_GPIO_Port GPIOA
#define MC2_Pin GPIO_PIN_12
#define MC2_GPIO_Port GPIOA
#define MD1_Pin GPIO_PIN_15
#define MD1_GPIO_Port GPIOA
#define MD2_Pin GPIO_PIN_4
#define MD2_GPIO_Port GPIOB
#define MA1_Pin GPIO_PIN_8
#define MA1_GPIO_Port GPIOB
#define MA2_Pin GPIO_PIN_9
#define MA2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

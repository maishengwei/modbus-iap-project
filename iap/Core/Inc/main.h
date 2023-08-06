/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
typedef  void (*pFunction)(void);
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
#define RS485_1_DE_Pin GPIO_PIN_0
#define RS485_1_DE_GPIO_Port GPIOB
#define RS485_1_RE_Pin GPIO_PIN_1
#define RS485_1_RE_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_14
#define LED3_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_15
#define LED2_GPIO_Port GPIOB
#define BOOT_SEL_PIN_Pin GPIO_PIN_5
#define BOOT_SEL_PIN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define LED3_ON     HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET)
#define LED3_OFF    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET)
#define LED3_TOGGLE HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin)
#define LED2_ON     HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)
#define LED2_OFF    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET)
#define LED2_TOGGLE HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin)
#define RS485_1_DE_ON   HAL_GPIO_WritePin(RS485_1_DE_GPIO_Port, RS485_1_DE_Pin, GPIO_PIN_SET)
#define RS485_1_DE_OFF  HAL_GPIO_WritePin(RS485_1_DE_GPIO_Port, RS485_1_DE_Pin, GPIO_PIN_RESET)
#define RS485_1_RE_ON   HAL_GPIO_WritePin(RS485_1_RE_GPIO_Port, RS485_1_RE_Pin, GPIO_PIN_SET)
#define RS485_1_RE_OFF  HAL_GPIO_WritePin(RS485_1_RE_GPIO_Port, RS485_1_RE_Pin, GPIO_PIN_RESET)
#define IS_RS485_1_DE_OFF (HAL_GPIO_ReadPin(RS485_1_DE_GPIO_Port, RS485_1_DE_Pin) == GPIO_PIN_RESET)
#define IS_RS485_1_RE_OFF (HAL_GPIO_ReadPin(RS485_1_RE_GPIO_Port, RS485_1_RE_Pin) == GPIO_PIN_RESET)
#define IS_BOOT_SEL_OFF (HAL_GPIO_ReadPin(BOOT_SEL_PIN_GPIO_Port, BOOT_SEL_PIN_Pin) == GPIO_PIN_RESET)

#define HIGH_BYTE(VALUE)        (((VALUE)>>8)&(0xFF))
#define LOW_BYTE(VALUE)         ((VALUE)&(0xFF))
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

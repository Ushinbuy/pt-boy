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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum {
	JOY_RIGHT,
	JOY_LEFT,
	JOY_DOWN,
	JOY_UP,
	JOY_CENT,
	BUTTON_A,
	BUTTON_B,
    BUTTONS_NUM,

	BUTTON_AB,
	BUTTONS_NOT_PRESSED
} PushedButton;
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
#define BTN_B_Pin GPIO_PIN_2
#define BTN_B_GPIO_Port GPIOA
#define BTN_B_EXTI_IRQn EXTI2_IRQn
#define DISP_DC_Pin GPIO_PIN_3
#define DISP_DC_GPIO_Port GPIOA
#define DISP_CS_Pin GPIO_PIN_4
#define DISP_CS_GPIO_Port GPIOA
#define BTN_A_Pin GPIO_PIN_5
#define BTN_A_GPIO_Port GPIOA
#define BTN_A_EXTI_IRQn EXTI9_5_IRQn
#define DISP_RES_Pin GPIO_PIN_0
#define DISP_RES_GPIO_Port GPIOB
#define JOY_B_Pin GPIO_PIN_10
#define JOY_B_GPIO_Port GPIOB
#define JOY_B_EXTI_IRQn EXTI15_10_IRQn
#define JOY_C_Pin GPIO_PIN_14
#define JOY_C_GPIO_Port GPIOB
#define JOY_C_EXTI_IRQn EXTI15_10_IRQn
#define JOY_D_Pin GPIO_PIN_15
#define JOY_D_GPIO_Port GPIOB
#define JOY_D_EXTI_IRQn EXTI15_10_IRQn
#define JOY_CENT_Pin GPIO_PIN_8
#define JOY_CENT_GPIO_Port GPIOB
#define JOY_CENT_EXTI_IRQn EXTI9_5_IRQn
#define JOY_A_Pin GPIO_PIN_9
#define JOY_A_GPIO_Port GPIOB
#define JOY_A_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */
void USB_CDC_RxHandler(uint8_t* Buf, uint32_t Len);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "snake.h"
#include "joystick_settings.h"
#include "pictures.h"
#include "processState.h"
#include "qr_pt.h"
#include "flashMem.h"
#include "editor.h"
#include <stdio.h>
#include <stdbool.h>
#include "button.h"
#include "usbd_cdc_if.h"
#include "i2c.h"
#include "createflags.h"
#include "bi_zone.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define WAIT_BEFORE_SLIDESHOW		5000
#define WAIT_1_SECOND				1000

typedef enum{
	START_SCREEN,
	SNAKE,
	PICTURE_VIEW,
	EDITOR,
	EDITOR_MENU,
	HELP,
	BIZONE_PICTURE
} PtBoyState;

static PtBoyState ptBoyState;
static ProcessState processState = PROCESS_START;

uint8_t flag1[FLAG1_LENGTH];
uint8_t flag2[FLAG2_LENGTH];
uint8_t key[KEY_LENGTH];
volatile static uint8_t isFlag2Selection = 0;
static uint8_t flag2FromUsr[FLAG2_LENGTH];
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi4;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI4_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */
HAL_StatusTypeDef checkLockLevel(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void ViewStartScreen(){
	ssd1306_Fill(Black);
	ssd1306_SetCursor(26, 4);
	ssd1306_WriteString("PLAY SNAKE TO", Font_6x8, White);
	ssd1306_SetCursor(8, 14);
	ssd1306_WriteString("UNLOCK NEW PICTURES", Font_6x8, White);
	ssd1306_SetCursor(4, 28);
	ssd1306_WriteString("Press A 1s > SNAKE", Font_6x8, White);
	ssd1306_SetCursor(4, 40);
	ssd1306_WriteString("Press B 1s > EDITOR", Font_6x8, White);
	ssd1306_SetCursor(4, 52);
	ssd1306_WriteString("Press A&B 1s > HELP", Font_6x8, White);
	ssd1306_UpdateScreen();
}

static void checkFlag2(){
	if(!isFlag2Selection)
		return;

	static uint8_t correctChar = 0;

	if(flag2[correctChar] == flag2FromUsr[correctChar]){
		correctChar++;
	}
	else{
		isFlag2Selection = 0;
		correctChar = 0;
		CDC_Transmit_FS((uint8_t*)"NOK", 3);
	}

	if(correctChar == FLAG2_LENGTH){
		isFlag2Selection = 0;
		correctChar = 0;
		taskSuccess(2);
		CDC_Transmit_FS((uint8_t*)"OK!", 3);
	}
}

static void FactoryTesting(PushedButton pushedButton){
	ssd1306_Fill(Black);
	char str[15];
	switch (pushedButton) {
	case JOY_CENT:
		sprintf(str, "BTN CENTER\r\n");
		break;
	case JOY_RIGHT:
		sprintf(str, "BTN RIGHT\r\n");
		break;
	case JOY_LEFT:
		sprintf(str, "BTN LEFT\r\n");
		break;
	case JOY_UP:
		sprintf(str, "BTN UP\r\n");
		break;
	case JOY_DOWN:
		sprintf(str, "BTN DOWN\r\n");
		break;
	case BUTTON_A:
		sprintf(str, "BTN A\r\n");
		break;
	case BUTTON_B:
		sprintf(str, "BTN B\r\n");
		break;
	case BUTTON_AB:
		sprintf(str, "BTN A&B\r\n");
		// start test flash for work
		writeLastScoresInMem(0);
		readLastScoresFromMem();
		HAL_Delay(100);
		// finish test flash
		break;
	default:
		return;
	}

	CDC_Transmit_FS((unsigned char*)str, strlen(str));

	ssd1306_SetCursor(0, 12);
	ssd1306_WriteString(str, Font_11x18, White);
	ssd1306_UpdateScreen();
}

static PushedButton ParseButtons() {
	bool notEditorNow = (ptBoyState != EDITOR && ptBoyState != EDITOR_MENU);
	bool notSnakeNow = (ptBoyState != SNAKE);
	if (buttonGetAction(JOY_RIGHT) != BUTTON_NONE) {
		return JOY_RIGHT;
	} else if (buttonGetAction(JOY_LEFT) != BUTTON_NONE) {
		return JOY_LEFT;
	} else if (buttonGetAction(JOY_UP) != BUTTON_NONE) {
		return JOY_UP;
	} else if (buttonGetAction(JOY_DOWN) != BUTTON_NONE) {
		return JOY_DOWN;
	} else if (buttonGetAction(JOY_CENT) != BUTTON_NONE) {
		return JOY_CENT;
	} else if (buttonGetAction(BUTTON_A) == BUTTON_SHORT_PRESS) {
		return BUTTON_A;
	} else if (buttonGetAction(BUTTON_A) == BUTTON_LONG_PRESS) {
		if (HAL_GPIO_ReadPin(BTN_B_GPIO_Port, BTN_B_Pin) == GPIO_PIN_RESET) {
			if (notEditorNow) {
				ptBoyState = HELP;
			}
			return BUTTON_AB;
		}
		if (notEditorNow && notSnakeNow) {
			processState = PROCESS_START;
			ptBoyState = SNAKE;
		}
		return BUTTON_A;
	} else if (buttonGetAction(BUTTON_B) == BUTTON_SHORT_PRESS) {
		return BUTTON_B;
	} else if (buttonGetAction(BUTTON_B) == BUTTON_LONG_PRESS) {
		if (HAL_GPIO_ReadPin(BTN_A_GPIO_Port, BTN_A_Pin) == GPIO_PIN_RESET) {
			if (notEditorNow) {
				ptBoyState = HELP;
			}
			return BUTTON_AB;
		}
		if (notEditorNow) {
			processState = PROCESS_START;
			ptBoyState = EDITOR;
		}
		return BUTTON_B;
	}

	return BUTTONS_NOT_PRESSED;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI4_Init();
  MX_TIM4_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

	uint32_t startScreenToSlideShowPause = 0;

	buttonInit();
	HAL_TIM_Base_Start_IT(&htim4);

	ssd1306_Init();

//#define CHECK_BTN_AND_DISPLAY
#define RELEASE_MAIN

#define SET_RDP_1
#if(defined SET_RDP_1 && defined RELEASE_MAIN)
	checkLockLevel();
#endif

#if defined(CHECK_BTN_AND_DISPLAY)

	ssd1306_Fill(White);
	HAL_Delay(300);
#endif

	picturesShowLogoAnimation();

	ptBoyState = START_SCREEN;
	startScreenToSlideShowPause = HAL_GetTick();

	HAL_I2C_EnableListen_IT(&hi2c1);
	generateFlags(flag1, key, flag2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		checkFlag2();

		buttonProcess();
		PushedButton currentButton = ParseButtons();
		buttonResetActions();

		static uint8_t isBizoneShow = 0;
		i2cHandle(&isBizoneShow);
		if (isBizoneShow == 1 && (ptBoyState != BIZONE_PICTURE)) {
			ptBoyState = BIZONE_PICTURE;
			processState = PROCESS_START;
		}

#if defined(CHECK_BTN_AND_DISPLAY)

		FactoryTesting(currentButton);
#elif defined(RELEASE_MAIN)
		switch (ptBoyState) {
		case START_SCREEN:
			ViewStartScreen();
			if ((HAL_GetTick() - startScreenToSlideShowPause)
					> WAIT_BEFORE_SLIDESHOW) {
				startScreenToSlideShowPause = HAL_GetTick();
				processState = PROCESS_START;
				ptBoyState = PICTURE_VIEW;
			}
			break;

		case PICTURE_VIEW:
			if (processState == PROCESS_START) {
				picturesSetupSlideShow();
				processState = PROCESS_EXECUTE;
			} else if (processState == PROCESS_EXECUTE) {
				picturesExecute(currentButton);
			}
			break;

		case SNAKE:
			switch (processState) {
			case PROCESS_START:
				snakeSetup();
				processState = PROCESS_EXECUTE;
				break;
			case PROCESS_EXECUTE:
				snakeSetState(currentButton);
				processState = snakeExecute();
				break;
			case PROCESS_FINISHED:
				HAL_Delay(3000);
				ptBoyState = PICTURE_VIEW;
				processState = PROCESS_START;
				break;
			default:
				break;
			}
			break;

		case EDITOR:
			if (processState == PROCESS_START) {
				editorInit();
				processState = PROCESS_EXECUTE;
			} else if (editorExecute(currentButton) == PROCESS_FINISHED) {
				ptBoyState = EDITOR_MENU;
			}
			break;
		case EDITOR_MENU: {
			ProcessState editorMenuState = editorMenu(currentButton);
			if (editorMenuState == PROCESS_SWITCH_TO_PROCESS1) {
				ptBoyState = EDITOR;
			} else if (editorMenuState == PROCESS_FINISHED) {
				ptBoyState = PICTURE_VIEW;
				processState = PROCESS_START;
			}
			break;
		}
		case HELP:
			ssd1306_Fill(Black);
			ssd1306_SetPicture(qr_pt);
			HAL_Delay(5000);
			startScreenToSlideShowPause = HAL_GetTick();
			ptBoyState = START_SCREEN;
			buttonResetActions();
			break;
		case BIZONE_PICTURE:
			if (isBizoneShow == 0) {
				ptBoyState = START_SCREEN;
				break;
			}
			ssd1306_Fill(Black);
			ssd1306_SetPicture(bi_zon4ik);
			break;
		default:
			break;
		}
#else
#error "You should define CHECK_BTN_AND_DISPLAY or RELEASE macro!"
#endif
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 16;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 96-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 1000-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DISP_DC_Pin|DISP_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BTN_B_Pin BTN_A_Pin */
  GPIO_InitStruct.Pin = BTN_B_Pin|BTN_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DISP_DC_Pin DISP_CS_Pin */
  GPIO_InitStruct.Pin = DISP_DC_Pin|DISP_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : DISP_RES_Pin */
  GPIO_InitStruct.Pin = DISP_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DISP_RES_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : JOY_B_Pin JOY_C_Pin JOY_D_Pin JOY_CENT_Pin
                           JOY_A_Pin */
  GPIO_InitStruct.Pin = JOY_B_Pin|JOY_C_Pin|JOY_D_Pin|JOY_CENT_Pin
                          |JOY_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == htim4.Instance) {
		buttonTimerProcess();
	}
}

HAL_StatusTypeDef checkLockLevel(void) {
	FLASH_OBProgramInitTypeDef ob;
	HAL_FLASHEx_OBGetConfig(&ob);
	if (ob.RDPLevel != OB_RDP_LEVEL_1) {
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		ob.OptionType = OPTIONBYTE_RDP;
		ob.RDPLevel = OB_RDP_LEVEL_1;

		if (HAL_FLASHEx_OBProgram(&ob) != HAL_OK) {
			HAL_FLASH_OB_Lock();
			HAL_FLASH_Lock();
			return HAL_ERROR;
		}

		HAL_FLASH_OB_Launch();

		/* We should not make it past the Launch, so lock
		 * flash memory and return an error from function
		 */
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
		return HAL_ERROR;
	}
	return HAL_OK;
}

void USB_CDC_RxHandler(uint8_t *Buf, uint32_t Len) {
	if (strncmp((char*) Buf, "fish", 4) == 0) {
		uint8_t name[NAME_LENGTH];
		readLastNameFromMem(name);
		CDC_Transmit_FS(name, NAME_LENGTH);
	} else if (strncmp((char*) Buf, "hamster", 7) == 0) {
		const uint8_t msgLength = 8;
		uint8_t outBuff[msgLength];
		for (uint8_t i = 0; i < msgLength; i++) {
			uint8_t currentNumber = i ^ 0x31;
			currentNumber &= 7;
			outBuff[i] = key[16 + currentNumber] ^ 0x52 - 2;
		}
		CDC_Transmit_FS(outBuff, msgLength);
	}

//#define DBG_INFO
#ifdef DBG_INFO
	else if(strncmp((char*)Buf, "info",sizeof("info")) == 0){
		uint8_t usbBuf[KEY_LENGTH + FLAG1_LENGTH + FLAG2_LENGTH];
		memcpy(usbBuf, key, 32);
		memcpy(&usbBuf[32], flag1, 32);
		memcpy(&usbBuf[64], flag2, 32);
		CDC_Transmit_FS(usbBuf, 96);
	}
#endif

	else if (strncmp("turtle", (char*) Buf, 6) == 0) {
		isFlag2Selection = 1;
		memcpy(flag2FromUsr, &Buf[6], FLAG2_LENGTH);
	} else if (strncmp((char*) Buf, "lizard", 6) == 0) {
		if (memcmp(&Buf[6], flag1, FLAG1_LENGTH) == 0) {
			CDC_Transmit_FS((uint8_t*) "OK!", 3);
			taskSuccess(3);
		} else {
			CDC_Transmit_FS((uint8_t*) "NOK", 3);
		}
	}

	memset(Buf, 0x0, APP_RX_DATA_SIZE);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

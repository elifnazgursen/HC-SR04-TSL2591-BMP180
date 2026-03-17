/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "TSL2591.h"
#include "HCSR04.h"
#include "BMP180.h"
#include "BuzzerLed.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */
uint32_t sensorTimer = 0;
uint32_t uartTimer = 0;

float temp = 0;
float pres = 0;
float alt = 0;

float last_temp = 0;
float last_pres = 0;

uint16_t ch0 = 0;
uint16_t ch1 = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* //float current_Temperature;
//float current_Pressure;
//float current_Altitude;
//
//float new_Temperature;
//float new_Pressure;
//float new_Altitude;
//
//float change_Temperature;
//float change_Pressure;
//float change_Altitude;

//char change_msg[] = "PRESS AGAIN FOR THE CHANGES\r\n";
//volatile uint8_t button_pressed = 0;

//uint8_t first_mes_taken = 0;
//volatile uint8_t uart_tx_ready = 1;

//uint8_t rx_byte;
//char rx_buffer[10];
//uint8_t rx_index = 0;
//uint8_t command_ready = 0; */



char uart_msg[128];
char temp_state[64] = "";
char pres_state[64] = "";

volatile uint8_t tim_flag = 0;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM1) {
		HCSR04_ProcessCapture(htim);
	}
}




/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	MX_DMA_Init();
	MX_TIM1_Init();
	MX_USART2_UART_Init();
	MX_I2C1_Init();
	MX_I2C2_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */

	/* //	HAL_UART_Receive_IT(&huart2, &rx_byte, 1); */


	HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);
	HAL_TIM_Base_Start(&htim1);
	HAL_TIM_Base_Start_IT(&htim2);

	if (TSL2591_Init() != HAL_OK) {
		while (1) {
		}
	}

	BMP180_Start();

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		if (HAL_GetTick() - sensorTimer >= 60) {
		    sensorTimer = HAL_GetTick();
		    HCSR04_Read();
		}

		BuzzerLed_Control(HCSR04_GetDistance());

		if (tim_flag) {
		    tim_flag = 0;

		    temp = BMP180_GetTemp();
		    pres = BMP180_GetPress(0);
		    alt  = BMP180_GetAlt(0);

		    char temp_state[10] = "";
		    char pres_state[10] = "";
		    char zone[10] = "";
		    int level = 0;

		    if (temp > last_temp) strcpy(temp_state, "WARMER");
		    else if (temp < last_temp) strcpy(temp_state, "COLDER");

		    if (pres > last_pres) strcpy(pres_state, "UP");
		    else if (pres < last_pres) strcpy(pres_state, "DOWN");

		    if (ReadRaw(&ch0, &ch1) == HAL_OK) {
		        if (ch0 >= 4000) level = 100;
		        else level = (ch0 * 100) / 4000;
		    }



		    uint32_t dist = HCSR04_GetDistance();

		    if (dist <= 10) strcpy(zone, "RED");
		    else if (dist <= 20) strcpy(zone, "YELLOW");
		    else strcpy(zone, "GREEN");

		    int len = snprintf(uart_msg, sizeof(uart_msg),
		        "BMP|TEMP:%.2f|TEMP_STATE:%s|PRES:%.2f|PRES_STATE:%s|ALT:%.2f\r\n",temp, temp_state, pres, pres_state, alt);
		    HAL_UART_Transmit(&huart2, (uint8_t*)uart_msg, len, HAL_MAX_DELAY);


		    len = snprintf(uart_msg, sizeof(uart_msg),
		        "TSL|CH0:%u|CH1:%u|LEVEL:%d\r\n",ch0, ch1, level);
		    HAL_UART_Transmit(&huart2, (uint8_t*)uart_msg, len, HAL_MAX_DELAY);


		    len = snprintf(uart_msg, sizeof(uart_msg),"HCSR04|DIST:%lu|ZONE:%s\r\n",dist, zone);
		    HAL_UART_Transmit(&huart2, (uint8_t*)uart_msg, len, HAL_MAX_DELAY);

		    last_temp = temp;
		    last_pres = pres;
		}

//		if (button_pressed || tim_flag) {
//
//			button_pressed = 0;
//			tim_flag = 0;
//
//			if (first_mes_taken == 0) {
//
//				current_Temperature = BMP180_GetTemp();
//				current_Pressure = BMP180_GetPress(0);
//				current_Altitude = BMP180_GetAlt(0);
//
//				HAL_TIM_Base_Start_IT(&htim2);
//
//				int len = snprintf(uart_msg, sizeof(uart_msg),
//						"Temperature: %.2f C\r\n"
//								"Pressure: %.2f Pa\r\n"
//								"Altitude: %.2f m\r\n\r\n", current_Temperature,
//						current_Pressure, current_Altitude);
//
//				HAL_UART_Transmit(&huart2, (uint8_t*) uart_msg, len,
//				HAL_MAX_DELAY); //verileri gönderdi
//
//				//				HAL_UART_Transmit(&huart2, (uint8_t*) change_msg, strlen(change_msg), HAL_MAX_DELAY); // yeni veriler için tekrar bas
//
//				first_mes_taken = 1;
//			}
//
//			else
//
//			{
//				new_Temperature = BMP180_GetTemp();
//				new_Pressure = BMP180_GetPress(0);
//				new_Altitude = BMP180_GetAlt(0);
//
//				//				change_Temperature = new_Temperature - current_Temperature;
//				//				change_Pressure = new_Pressure - current_Pressure;
//
//				if (new_Temperature > current_Temperature) {
//					strcpy(temp_state, "WARMER");
//				} else if (new_Temperature < current_Temperature) {
//					strcpy(temp_state, "COLDER");
//				} else {
//					strcpy(temp_state, " ");
//				}
//
//				if (new_Pressure > current_Pressure + 0.5) {
//					strcpy(pres_state, "UP");
//				} else if (new_Pressure < current_Pressure + 0.5) {
//					strcpy(pres_state, "DOWN");
//				}
//
//				int len = snprintf(uart_msg, sizeof(uart_msg),
//						"Temperature: %.2f C  (%s) \r\n"
//								"Pressure: %.2f Pa   (%s)\r\n"
//								"Altitude: %.2f m\r\n\r\n", new_Temperature,
//						temp_state, new_Pressure, pres_state, new_Altitude);
//
//				HAL_UART_Transmit(&huart2, (uint8_t*) uart_msg, len,
//				HAL_MAX_DELAY);
//
//				current_Temperature = new_Temperature;
//				current_Pressure = new_Pressure;
//				current_Altitude = new_Altitude;
//
//				new_Temperature = 0;
//				new_Pressure = 0;
//				new_Altitude = 0;
//
//			}
//
//		}

	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C2_Init(void) {

	/* USER CODE BEGIN I2C2_Init 0 */

	/* USER CODE END I2C2_Init 0 */

	/* USER CODE BEGIN I2C2_Init 1 */

	/* USER CODE END I2C2_Init 1 */
	hi2c2.Instance = I2C2;
	hi2c2.Init.ClockSpeed = 100000;
	hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C2_Init 2 */

	/* USER CODE END I2C2_Init 2 */

}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_IC_InitTypeDef sConfigIC = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 168 - 1;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 65535;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_IC_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
	sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
	sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
	sConfigIC.ICFilter = 0;
	if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 8400 - 1;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 30000 - 1;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, YE__L_Pin | SARI_Pin | KIRMIZI_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOE, BUZZER_PIN_HCSR04_Pin | TRIG_HCSR04_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin : PA0 */
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : YE__L_Pin SARI_Pin KIRMIZI_Pin */
	GPIO_InitStruct.Pin = YE__L_Pin | SARI_Pin | KIRMIZI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : BUZZER_PIN_HCSR04_Pin TRIG_HCSR04_Pin */
	GPIO_InitStruct.Pin = BUZZER_PIN_HCSR04_Pin | TRIG_HCSR04_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART2)
//    {
//        if (rx_byte == '\n' || rx_byte == '\r')
//        {
//            rx_buffer[rx_index] = '\0';
//            rx_index = 0;
//            command_ready = 1;
//        }
//        else
//        {
//            rx_buffer[rx_index++] = rx_byte;
//        }
//
//        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
//    }
//}

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//	if (GPIO_Pin == GPIO_PIN_0) {
//		button_pressed = 1;
//	}
//}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		tim_flag = 1;
//        HAL_TIM_Base_Start_IT(&htim2);
	}
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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

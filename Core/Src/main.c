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
#include "BMP180.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
	#define BUZZER_PIN GPIO_PIN_8
	#define BUZZER_PORT GPIOE
	#define TRIG_PIN GPIO_PIN_10
	#define TRIG_PORT GPIOE

	#define ADDR (0x29 << 1) // sensörün i2c 7 bit adresi (0x29), 1 kere sola kayd. çünk son bit read/write kullanılır gnld. biz HAL e kaydırarak veririz
	#define CMD_BIT 0xA0 // sensöre register erişimi yaparken eklenen özel komut biti

	#define ENABLE_REG   0x00 // açıl çalış ölçüme başla
	#define CONFIG_REG   0x01 // config reg i, ne kadar hassas ölçsün ya da ne kadar ölçsün
	#define STATUS_REG   0x13 // status - veri hazır mı? ölçüm olmuş mu?

	#define C0DATAL_REG  0x14 // channel 0 ın alt byte ı
	#define C1DATAL_REG  0x16 // channel 1 in alt byte ı



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

/* USER CODE BEGIN PV */


	uint32_t IC_Val1 = 0;
	uint32_t IC_Val2 = 0;
	uint32_t Difference = 0;
	uint32_t Is_First_Captured = 0;
	uint32_t Distance = 0;

	uint32_t sensorTimer = 0;
	uint32_t buzzerTimer = 0;

	uint32_t buzzerInterval = 0;
	uint8_t  buzzerState = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

float current_Temperature;
float current_Pressure;
float current_Altitude;

float new_Temperature;
float new_Pressure;
float new_Altitude;

float change_Temperature;
float change_Pressure;
float change_Altitude;

char uart_msg[128];
//char change_msg[] = "PRESS AGAIN FOR THE CHANGES\r\n";

char temp_state[64] = "";
char pres_state[64] = "";

volatile uint8_t button_pressed = 0;
volatile uint8_t tim_flag = 0;

uint8_t first_mes_taken = 0;

//volatile uint8_t uart_tx_ready = 1;

		//FONK. GÖREVİ: SENSÖR İÇİNDEKİ BİR REG E VERİ YAZMAK
		HAL_StatusTypeDef WriteReg(uint8_t reg, uint8_t data) // hangi reg e ne yazacağız (data) ?
		{
			uint8_t buf[2];

			buf[0] = CMD_BIT | reg; // gidilecek register (Mesela reg = 0x00 ise: 0xA0 | 0x00 = 0xA0 ben ENABLE REG e gidiyorum demek)

			buf[1] = data; // yazılacak komut-veri

			// I2C ÜZERİNDEN VERİ YOLLAMA
			// (hangi i2c?,kiminle konuşacağız?, hangi veriyi göndereceğiz?, kaç byte göndericez?, ne kadar bekliyim?)

			return HAL_I2C_Master_Transmit(&hi2c1, ADDR, buf, 2, HAL_MAX_DELAY); // neden return? -> HAL fonksiyonundan gelen sonucu aynen geri gönderiyoruz
		}




		//FONK. GÖREVİ: SENSÖRDEN 1 BYTE VERİ OKUMAK
		HAL_StatusTypeDef ReadReg(uint8_t reg, uint8_t *data) // hangi reg okunsun? okunan veri nereye yazılsın?
		{
		    uint8_t cmd = CMD_BIT | reg; // hangi reg i okumak istediğimi söylüyorum, önce red adr gönderiyoruz

		    if (HAL_I2C_Master_Transmit(&hi2c1, ADDR, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
		        return HAL_ERROR; // ilk gönderim başarısızsa veri okumaya devam etme


		    // veriyi okuyan satır ( i2c üzerinden karşı taraftan veri alır)
		    // (i2c1 kullan, sensörden al, aldığın veriyi dataya koy, 1 byte al, bekle)

		    return HAL_I2C_Master_Receive(&hi2c1,ADDR, data, 1, HAL_MAX_DELAY);
		}




		//FONK. GÖREVİ: sensörden 2 byte okuyup bunu 16 bit sayı haline getirmek
		HAL_StatusTypeDef Read2Bytes(uint8_t startReg, uint16_t *value)
		{
		    uint8_t cmd = CMD_BIT | startReg;
		    uint8_t buf[2]; // 2 byte geçici olarak burada saklanır

		    // şu register’dan okumaya başlayacağım
		    if (HAL_I2C_Master_Transmit(&hi2c1, ADDR, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
		        return HAL_ERROR;

		    // Sonra sensörden 2 byte alıyoruz.
		    if (HAL_I2C_Master_Receive(&hi2c1, ADDR, buf, 2, HAL_MAX_DELAY) != HAL_OK)
		        return HAL_ERROR;

		    *value = (uint16_t)(buf[1] << 8) | buf[0]; // Burada iki ayrı byte’ı birleştirip tek sayı yapıyoruz.
		    return HAL_OK;
		}



		//FONK. GÖREVİ: SENSÖRÜ BAŞLATMAK. sensör doğru mu, bağlı mı, çalışmaya hazır mı?
		// sensörü başlat -> sonuç döndür
		HAL_StatusTypeDef TSL2591_Init(void)
		{
		    uint8_t id = 0; // sensör kimlik bilgisi

		    if (ReadReg(0x12, &id) != HAL_OK) // sensörün id reg ini okuyoruz datasheetten. gerçekten tsl mi?
		        return HAL_ERROR; // eğer okunmuyorsa dur

		    if (id != 0x50)  //okuduğum kimlik 0x50 değilse bu beklediğim sensör değil
		        return HAL_ERROR;

		    if (WriteReg(ENABLE_REG, 0x03) != HAL_OK) // SENSÖRÜ AÇIYORUZ ENABLE_REG = 0x00 Bu register’a 0x03 yazıyoruz. power on
		        return HAL_ERROR;

		    if (WriteReg(CONFIG_REG, 0x10) != HAL_OK) // Burada ayar yapıyoruz. 0x10 ile örnek bir gain/integration ayarı veriyoruz.
		        return HAL_ERROR;

		    HAL_Delay(120);

		    return HAL_OK; //Her şey yolunda gittiyse: sensör bulundu - sensör açıldı -  ayar yapıldı - o zaman başarıyla çık.
		}




		//FONK. GÖREVİ: sensörden gerçek ışık verilerini almak
		HAL_StatusTypeDef ReadRaw(uint16_t *ch0, uint16_t *ch1)
		{
		    uint8_t status = 0;

		    if (ReadReg(STATUS_REG, &status) != HAL_OK)  // veri hazır mı? sensör ölçümü bitirmiş mi?
		        return HAL_ERROR;

		    if ((status & 0x01) == 0) // Datasheette AVALID biti veri geçerli mi sorusunu cevaplar. 0 ise demek ki veri henüz hazır değil.
		        return HAL_BUSY; // “bekle, daha ölçüm bitmedi”

		    if (Read2Bytes(C0DATAL_REG, ch0) != HAL_OK) //görünür + IR karışımı
		        return HAL_ERROR;

		    if (Read2Bytes(C1DATAL_REG, ch1) != HAL_OK) //daha çok IR
		        return HAL_ERROR;

		    return HAL_OK;
		}




	void delay(uint8_t time)
	{
		__HAL_TIM_SET_COUNTER(&htim1, 0);  //reset the counter

		while (__HAL_TIM_GET_COUNTER (&htim1) < time); //This function gives us the desired delay in microseconds.

	}


	void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
	{
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)  //  interrupt source is channel1
		{

			if (Is_First_Captured==0) // if the first value is not captured
			{
				IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1); // read the first value

				Is_First_Captured = 1;  // set the first captured as true


				// Now change the polarity to falling edge
				__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
			}



			else if (Is_First_Captured==1)   // if the first is already captured
			{
				IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);  // read second value
				__HAL_TIM_SET_COUNTER(htim, 0);  // reset the counter



				if (IC_Val2 > IC_Val1)
				{
					Difference = IC_Val2-IC_Val1;
				}


				else if (IC_Val1 > IC_Val2)
				{
					Difference = (0xffff - IC_Val1) + IC_Val2;
				}

				Distance = Difference * .034/2;
				Is_First_Captured = 0; // set it back to false


				// set polarity to rising edge
				__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
				__HAL_TIM_DISABLE_IT(&htim1, TIM_IT_CC1);
			}
		}
	}

	void HCSR04_Read (void)
	{
		HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);  // pull the TRIG pin HIGH
		delay(10);  // wait for 10 us

		HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);  // pull the TRIG pin low

		__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC1);
	}


	void Buzzer_Control(void)
	{
		if (Distance > 20)
		{
			buzzerInterval = 0;
			buzzerState = 0;
			HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

		}

		else if (Distance > 15 && Distance <= 20)
		{
			buzzerInterval = 300;
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

		}

		else if (Distance > 10 && Distance <= 15)
		{
			buzzerInterval = 120;
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
		}

		else
		{
			buzzerInterval = 0;
			buzzerState = 1;
			HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

		}

		if (buzzerInterval > 0)
		{
			if (HAL_GetTick() - buzzerTimer >= buzzerInterval)
			{
				buzzerTimer = HAL_GetTick();

				buzzerState = !buzzerState;

				HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN,
								  buzzerState ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
		}
	}


	// ışık verileri
	uint16_t ch0 = 0;
	uint16_t ch1 = 0;

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
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	  HAL_TIM_IC_Start_IT(&htim1,TIM_CHANNEL_1);
	  HAL_TIM_Base_Start(&htim1);

	  if (TSL2591_Init() != HAL_OK)
	      {
	          while (1)
	          {
	          }
	      }

	  BMP180_Start();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	  while (1)
	  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		  if (HAL_GetTick() - sensorTimer >= 60)
			  {
				  sensorTimer = HAL_GetTick();
				  HCSR04_Read();
			  }

		  Buzzer_Control();

		  if (ReadRaw(&ch0, &ch1) == HAL_OK)
		          {
		          }

		          HAL_Delay(200);

		          if (button_pressed || tim_flag)
		          		{

		          			button_pressed = 0;
		          			tim_flag = 0;

		          			if (first_mes_taken == 0)
		          			{

		          				current_Temperature = BMP180_GetTemp();
		          				current_Pressure = BMP180_GetPress(0);
		          				current_Altitude = BMP180_GetAlt(0);

		          				HAL_TIM_Base_Start_IT(&htim2);

		          				int len = snprintf(uart_msg, sizeof(uart_msg),
		          						"Temperature: %.2f C\r\n"
		          						"Pressure: %.2f Pa\r\n"
		          						"Altitude: %.2f m\r\n\r\n", current_Temperature,current_Pressure, current_Altitude);

		          				HAL_UART_Transmit(&huart2, (uint8_t*) uart_msg, len, HAL_MAX_DELAY); //verileri gönderdi

		          //				HAL_UART_Transmit(&huart2, (uint8_t*) change_msg, strlen(change_msg), HAL_MAX_DELAY); // yeni veriler için tekrar bas


		          				first_mes_taken = 1;
		          			}

		          			else

		          			{
		          				new_Temperature = BMP180_GetTemp();
		          				new_Pressure = BMP180_GetPress(0);
		          				new_Altitude = BMP180_GetAlt(0);

		          //				change_Temperature = new_Temperature - current_Temperature;
		          //				change_Pressure = new_Pressure - current_Pressure;

		          				if (new_Temperature > current_Temperature)
		          				{
		          					strcpy(temp_state, "WARMER");
		          				}
		          				else if (new_Temperature < current_Temperature)
		          				{
		          					strcpy(temp_state, "COLDER");
		          				}
		          				else
		          				{
		          					strcpy(temp_state, " ");
		          				}




		          				if (new_Pressure > current_Pressure + 0.5)
		          				{
		          					strcpy(pres_state, "UP");
		          				}
		          				else if (new_Pressure < current_Pressure + 0.5)
		          				{
		          					strcpy(pres_state, "DOWN");
		          				}



		          				int len = snprintf(uart_msg, sizeof(uart_msg),
		          										"Temperature: %.2f C  (%s) \r\n"
		          										"Pressure: %.2f Pa   (%s)\r\n"
		          										"Altitude: %.2f m\r\n\r\n", new_Temperature,temp_state, new_Pressure, pres_state, new_Altitude);

		          				HAL_UART_Transmit(&huart2, (uint8_t*) uart_msg, len, HAL_MAX_DELAY);

		          				current_Temperature = new_Temperature;
		          				current_Pressure = new_Pressure ;
		          				current_Altitude = new_Altitude ;

		          				new_Temperature = 0;
		          				new_Pressure = 0;
		          				new_Altitude = 0;



		          			}




		          		}


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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
  hi2c1.Init.OwnAddress1 = 0;
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
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

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
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
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
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 168-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
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
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8400-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 30000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
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
static void MX_USART2_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, YE__L_Pin|SARI_Pin|KIRMIZI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, BUZZER_PIN_HCSR04_Pin|TRIG_HCSR04_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : YE__L_Pin SARI_Pin KIRMIZI_Pin */
  GPIO_InitStruct.Pin = YE__L_Pin|SARI_Pin|KIRMIZI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BUZZER_PIN_HCSR04_Pin TRIG_HCSR04_Pin */
  GPIO_InitStruct.Pin = BUZZER_PIN_HCSR04_Pin|TRIG_HCSR04_Pin;
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
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_0) {
		button_pressed = 1;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        tim_flag = 1;
        HAL_TIM_Base_Start_IT(&htim2);
    }
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

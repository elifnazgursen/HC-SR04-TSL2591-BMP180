
   #include "BuzzerLed.h"

	#define BUZZER_PIN GPIO_PIN_8
	#define BUZZER_PORT GPIOE

	uint32_t buzzerTimer = 0;
	uint32_t buzzerInterval = 0;
	uint8_t  buzzerState = 0;

	void BuzzerLed_Control(uint32_t distance)
		{

			if (distance > 20)
			{
				buzzerInterval = 0;
				buzzerState = 0;
				HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

			}

			else if (distance > 15 && distance <= 20)
			{
				buzzerInterval = 300;
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

			}

			else if (distance > 10 && distance <= 15)
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

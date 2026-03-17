




	#ifndef INC_HCSR04_H_
	#define INC_HCSR04_H_

	#include "main.h"

	void HCSR04_Read(void);
	void HCSR04_DelayUs(uint8_t time);
	void HCSR04_ProcessCapture(TIM_HandleTypeDef *htim);

	uint32_t HCSR04_GetDistance(void);

	#endif /* INC_HCSR04_H_ */

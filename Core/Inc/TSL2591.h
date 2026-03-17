


#ifndef INC_TSL2591_H_
#define INC_TSL2591_H_

#include "main.h"

HAL_StatusTypeDef TSL2591_Init(void);
HAL_StatusTypeDef ReadRaw(uint16_t *ch0, uint16_t *ch1);

#endif /* INC_TSL2591_H_ */

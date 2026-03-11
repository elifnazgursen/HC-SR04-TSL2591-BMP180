/**
 ******************************************************************************

 BMP180 LIBRARY for STM32 using I2C
 Author:   ControllersTech
 Updated:  26/07/2020

 ******************************************************************************
 Copyright (C) 2017 ControllersTech.com

 This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
 of the GNU General Public License version 3 as published by the Free Software Foundation.
 This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
 or indirectly by this software, read more about this on the GNU General Public License.

 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include "math.h"
#include "BMP180.h"

BMP180_t bmp180;

BMP180_CalibData *cal = &bmp180.calib;
BMP180_RawData   *raw = &bmp180.raw;
BMP180_CalcData  *c   = &bmp180.calc;
BMP180_Result    *res = &bmp180.result;

extern I2C_HandleTypeDef hi2c1; // I2C Callbacklerinin yazılı olduğu fonksiyon

#define BMP180_I2C &hi2c1       // I2C1 e bağlı sensörü tanımlıyor, I2C1 i kullanıyoruz

#define BMP180_ADDRESS 0xEE     // BMP180 in ölçüm başlatma/okuma adresi

#define atmPress 101325         // Pa

void read_calliberation_data(void) {

	uint8_t Callib_Data[22] = { 0 };

	uint16_t Callib_Start = 0xAA;

	HAL_I2C_Mem_Read(BMP180_I2C, BMP180_ADDRESS, Callib_Start, 1, Callib_Data, 22, HAL_MAX_DELAY);

	cal->AC1 = ((Callib_Data[0] << 8) | Callib_Data[1]);
	cal->AC2 = ((Callib_Data[2] << 8) | Callib_Data[3]);
	cal->AC3 = ((Callib_Data[4] << 8) | Callib_Data[5]);
	cal->AC4 = ((Callib_Data[6] << 8) | Callib_Data[7]);
	cal->AC5 = ((Callib_Data[8] << 8) | Callib_Data[9]);
	cal->AC6 = ((Callib_Data[10] << 8) | Callib_Data[11]);

	cal->B1 = ((Callib_Data[12] << 8) | Callib_Data[13]);
	cal->B2 = ((Callib_Data[14] << 8) | Callib_Data[15]);

	cal->MB = ((Callib_Data[16] << 8) | Callib_Data[17]);
	cal->MC = ((Callib_Data[18] << 8) | Callib_Data[19]);
	cal->MD = ((Callib_Data[20] << 8) | Callib_Data[21]);

}

// Get uncompensated Temp

uint16_t Get_UTemp(void) {

	uint8_t datatowrite = 0x2E;
	uint8_t Temp_RAW[2] = { 0 };

	HAL_I2C_Mem_Write(BMP180_I2C, BMP180_ADDRESS, 0xF4, 1, &datatowrite, 1, 1000);
	HAL_Delay(5);  // wait 4.5 ms

	HAL_I2C_Mem_Read(BMP180_I2C, BMP180_ADDRESS, 0xF6, 1, Temp_RAW, 2, 1000);

	return ((Temp_RAW[0] << 8) + Temp_RAW[1]);
}


float BMP180_GetTemp(void)
{
	raw->UT = Get_UTemp();
	c->X1 = ((raw->UT - cal->AC6)* (cal->AC5 / (pow(2, 15))));
	c->X2 = ((cal->MC * (pow(2, 11)))/ (c->X1 + cal->MD));
	c->B5 = c->X1 + c->X2;
	res->temp  = (c->B5 + 8) / (pow(2, 4));

	return res->temp / 10.0;
}

// Get uncompensated Pressure

uint32_t Get_UPress(int oss)   // oversampling setting 0,1,2,3
{
	uint8_t datatowrite = 0x34 + (oss << 6);
	uint8_t Press_RAW[3] = { 0 };

	HAL_I2C_Mem_Write(BMP180_I2C, BMP180_ADDRESS, 0xF4, 1, &datatowrite, 1,
			1000);

	switch (oss) {
	case (0):
		HAL_Delay(5);
		break;
	case (1):
		HAL_Delay(8);
		break;
	case (2):
		HAL_Delay(14);
		break;
	case (3):
		HAL_Delay(26);
		break;
	}

	HAL_I2C_Mem_Read(BMP180_I2C, BMP180_ADDRESS, 0xF6, 1, Press_RAW, 3, 1000);

	return (((Press_RAW[0] << 16) + (Press_RAW[1] << 8) + Press_RAW[2]) >> (8 - oss));
}

float BMP180_GetPress(int oss)
{
	raw->UP = Get_UPress(oss);
	c->X1 = ((raw->UT - cal->AC6) * (cal->AC5 / (pow(2, 15))));
	c->X2 = ((cal->MC * (pow(2, 11))) / (c->X1 + cal->MD));
	c->B5 = c->X1 + c->X2;
	c->B6 = c->B5 - 4000;
	c->X1 = (cal->B2 * (c->B6 * c->B6 / (pow(2, 12)))) / (pow(2, 11));
	c->X2 = cal->AC2 * c->B6 / (pow(2, 11));
	c->X3 = c->X1 + c->X2;
	c->B3 = (((cal->AC1 * 4 + c->X3) << oss) + 2) / 4;
	c->X1 = cal->AC3 * c->B6 / pow(2, 13);
	c->X2 = (cal->B1 * (c->B6 * c->B6 / (pow(2, 12)))) / (pow(2, 16));
	c->X3 = ((c->X1 + c->X2) + 2) / pow(2, 2);
	c->B4 = cal->AC4 * (unsigned long) (c->X3 + 32768) / (pow(2, 15));
	c->B7 = ((unsigned long) raw->UP - c->B3) * (50000 >> oss);

	if (c->B7 < 0x80000000)
		res->pres = (c->B7 * 2) / c->B4;
	else
		res->pres = (c->B7 / c->B4) * 2;

	c->X1 = (res->pres / (pow(2, 8))) * (res->pres / (pow(2, 8)));
	c->X1 = (c->X1 * 3038) / (pow(2, 16));
	c->X2 = (-7357 * res->pres) / (pow(2, 16));

	res->pres = res->pres + (c->X1 + c->X2 + 3791) / (pow(2, 4));

	return res->pres;
}

float BMP180_GetAlt(int oss)
{
	BMP180_GetPress(oss);

	return 44330 * (1 - (pow((res->pres / (float) atmPress), 0.19029495718)));
}

void BMP180_Start(void)
{
	read_calliberation_data();
}


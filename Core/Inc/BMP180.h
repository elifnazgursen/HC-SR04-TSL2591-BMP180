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

#ifndef _BMP180_H_
#define _BMP180_H_


#include "stm32f4xx_hal.h"

// Calibration data

typedef struct
{
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;

    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;

    int16_t B1;
    int16_t B2;

    int16_t MB;
    int16_t MC;
    int16_t MD;

} BMP180_CalibData;


// Raw data

typedef struct
{
    int32_t UT;
    int32_t UP;
    int8_t oss;

} BMP180_RawData;


// Calculation variables

typedef struct
{
    int32_t X1;
    int32_t X2;
    int32_t X3;

    int32_t B3;
    int32_t B5;
    int32_t B6;

    uint32_t B4;
    uint32_t B7;

} BMP180_CalcData;


// Final results

typedef struct
{
    int32_t temp;
    int32_t pres;

} BMP180_Result;


// Main sensor structure

typedef struct
{
    BMP180_CalibData calib;
    BMP180_RawData raw;
    BMP180_CalcData calc;
    BMP180_Result result;

} BMP180_t;


void BMP180_Start (void);

float BMP180_GetTemp (void);

float BMP180_GetPress (int oss);

float BMP180_GetAlt (int oss);

#endif /* INC_BMP180_H_ */

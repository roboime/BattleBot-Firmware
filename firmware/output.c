//
// output.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo possui as funções que operam sobre os
// motores, para colocá-los em funcionamento ou não
//

#include "default.h"

#define MOTOR_MAX_POWER 252
#define MOTOR_MIN_POWER 8
#define CLAMP(p,m) do { if (p > (m)) p = (m); else if (p < -(m)) p = -(m); } while (0)

void motor_set_power_left(int16_t power)
{
	CLAMP(power, MOTOR_MAX_POWER);
	
	if (power >= MOTOR_MIN_POWER)
	{
		OCR0A = power;
		OCR0B = 0;
	}
	else if (power <= -MOTOR_MIN_POWER)
	{
		OCR0A = 0;
		OCR0B = -power;
	}
	else
	{
		OCR0A = 0;
		OCR0B = 0;
	}
}

void motor_set_power_right(int16_t power)
{
	CLAMP(power, MOTOR_MAX_POWER);
	
	if (power >= MOTOR_MIN_POWER)
	{
		OCR1AL = power;
		OCR1BL = 0;
	}
	else if (power <= -MOTOR_MIN_POWER)
	{
		OCR1AL = 0;
		OCR1BL = -power;
	}
	else
	{
		OCR1AL = 0;
		OCR1BL = 0;
	}
}

void led_set(uint8_t on)
{
	if (on) PORTC |= 1;
	else PORTC &= ~1;
}

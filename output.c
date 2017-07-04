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

#define MOTOR_MAX_POWER 250
#define MOTOR_MIN_POWER 8

static volatile uint8_t esc_power = 123;

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

void esc_set_power(int16_t power)
{
	CLAMP(power, 244);
	esc_power = 123 + power;
	flags |= ESC_AVAILABLE;
}

void led_set(uint8_t on)
{
	if (on) PORTB |= _BV(5);
	else PORTB &= ~_BV(5);
}

ISR (TIMER2_COMPA_vect, ISR_NAKED)
{
	PORTD &= ~_BV(4);
	asm volatile("reti");
}

ISR (TIMER2_OVF_vect)
{
	static uint8_t counter = 1;
	switch (counter)
	{
		case 19: TIMSK2 &= ~_BV(OCIE2A); break;
		case 1: if (flags & ESC_AVAILABLE) PORTD |= _BV(4); break;
		case 0:
			counter = 20;
			TIFR2 = _BV(OCF2A); // des-seta a flag de comparação
			OCR2A = esc_power; // seta o valor para a comparação
			TIMSK2 |= _BV(OCIE2A); // liga o interrupt na comparação
			break;
		default: break;
	}
	counter--;
}

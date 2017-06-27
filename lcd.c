//
// spi.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo define as funções e dados necessários
// para interfacear com o módulo LCD
//

#include "default.h"
#include <util/delay.h>

#define E_PORT PORTB
#define E_BIT 0

#define B1_PORT PORTB
#define B1_BIT 3
#define B2_PORT PORTB
#define B2_BIT 4
#define B3_PORT PORTB
#define B3_BIT 5
#define B4_PORT PORTD
#define B4_BIT 4

#define RS_PORT PORTD
#define RS_BIT 7

#define EM_INCR _BV(1)
#define EM_DECR 0
#define EM_DISPLAY_SHIFT _BV(0)

#define DC_DISPLAY _BV(2)
#define DC_CURSOR _BV(1)
#define DC_CURSOR_BLINKING _BV(0)

#define MCS_RIGHT _BV(2)
#define MCS_LEFT 0

#define FS_8BIT _BV(4)
#define FS_4BIT 0
#define FS_2LINES _BV(3)
#define FS_1LINE 0
#define FS_F5x10 _BV(2)
#define FS_F5x8 0

inline static void lcd_write4(uint8_t data)
{
	if (data & B1) B1_PORT |= _BV(B1_BIT);
	else B1_PORT &= ~_BV(B1_BIT);
	if (data & B10) B2_PORT |= _BV(B2_BIT);
	else B2_PORT &= ~_BV(B2_BIT);
	if (data & B100) B3_PORT |= _BV(B3_BIT);
	else B3_PORT &= ~_BV(B3_BIT);
	if (data & B1000) B4_PORT |= _BV(B4_BIT);
	else B4_PORT &= ~_BV(B4_BIT);
	
	E_PORT &= ~_BV(E_BIT);
	_delay_us(1);
	E_PORT |= _BV(E_BIT);
	_delay_us(1);
	E_PORT &= ~_BV(E_BIT);
	_delay_us(40);
}

inline static void lcd_write_ir(uint8_t data)
{
	RS_PORT &= ~_BV(RS_BIT);
	lcd_write4(data >> 4);
	lcd_write4(data);
}

inline static void lcd_write_dr(uint8_t data)
{
	RS_PORT |= _BV(RS_BIT);
	lcd_write4(data >> 4);
	lcd_write4(data);
}

void lcd_clear()
{
	lcd_write_ir(B1);
	_delay_ms(1.5);
	wdt_reset();
}

inline static void lcd_return_home()
{
	lcd_write_ir(B10);
	_delay_ms(1.5);
	wdt_reset();
}

inline static void lcd_entry_mode_set(uint8_t val)
{
	lcd_write_ir(B100 | (val & B11));
}

inline static void lcd_display_control(uint8_t val)
{
	lcd_write_ir(B1000 | (val & B111));
}

inline static void lcd_move_cursor(uint8_t val)
{
	lcd_write_ir(B10000 | (val & B100));
}

inline static void lcd_display_shift(uint8_t val)
{
	lcd_write_ir(B10100 | (val & B100));
}

inline static void lcd_function_set(uint8_t val)
{
	lcd_write_ir(B100000 | (val & B11100));
}

inline static void lcd_set_ddram_addr(uint8_t addr)
{
	lcd_write_ir(B10000000 | (addr & B1111111));
}

void lcd_init()
{
	// Rotina de inicialização do LCD
	// Primeira tentativa
	RS_PORT &= ~_BV(RS_BIT);
	lcd_write4(3);
	_delay_ms(4.5);
	wdt_reset();
	
	// Segunda tentativa
	lcd_write4(3);
	_delay_ms(4.5);
	wdt_reset();
	
	// Terceira tentativa
	lcd_write4(3);
	_delay_us(40);
	_delay_us(40);
	_delay_us(40);
	wdt_reset();

	// Interface de 4 bits
	lcd_write4(2);
	lcd_function_set(FS_4BIT | FS_2LINES | FS_F5x8);
	
	// Configuração do LCD
	lcd_display_control(0);
	lcd_entry_mode_set(EM_INCR);
	
	// "Limpa" a tela
	lcd_clear();
	lcd_display_control(DC_DISPLAY);
}

void lcd_write_chars(uint8_t r, uint8_t c, const char *data, uint8_t size)
{
	lcd_entry_mode_set(EM_INCR);	
	lcd_set_ddram_addr(0x40 * r + c);

	for (uint8_t i = 0; i < size; i++)
		lcd_write_dr(data[i]);
}

static void lcd_write_uint16_sgn(uint8_t r, uint8_t c, uint16_t value, char sgn)
{
	lcd_entry_mode_set(EM_DECR);
	lcd_set_ddram_addr(0x40 * r + c + 3);
	
	int8_t sgn_flag = 0;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (value == 0)
		{
			lcd_write_dr(sgn_flag ? ' ' : sgn);
			sgn_flag = 1;
		}
		else lcd_write_dr('0' + (value % 10));
		value /= 10;
	}
	
	if (sgn_flag == 0) lcd_write_dr(sgn);
}

void lcd_write_int16(uint8_t r, uint8_t c, int16_t value)
{
	if (value > 0) lcd_write_uint16_sgn(r, c, value, 'p');
	else if (value < 0) lcd_write_uint16_sgn(r, c, -value, 'N');
	else lcd_write_uint16_sgn(r, c, 0, '0');
}

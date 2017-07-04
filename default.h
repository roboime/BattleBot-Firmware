//
// default.h
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo possui importantes definições, que devem
// estar presentes em TODOS os módulos, porque são vitais
// para o bom functionamento do programa
//

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "binaries.h"

#define SETMIN(p,m) do { if (p > -(m) && p < (m)) p = 0; } while (0)
#define CLAMP(p,m) do { if (p > (m)) p = (m); else if (p < -(m)) p = -(m); } while (0)

#define EXECUTE_ENC 1
#define RECV_AVAL0 2
#define RECV_AVAL1 4
#define RECV_AVAL2 8
#define RECV_AVAL3 16
#define RECV_AVAL4 32
#define TWI_PREV_START 64
#define ESC_AVAILABLE 128
#define EXECUTE_RECV (RECV_AVAL0|RECV_AVAL1|RECV_AVAL2|RECV_AVAL3|RECV_AVAL4)
#define flags GPIOR0

// A gente usa a funçaão wdt_off() porque a wdt_disable() possui erros
void wdt_off();

void input_init();
void input_read_enc();
void input_read_recv();

int16_t recv_get_ch(uint8_t ch);
uint8_t recv_online();
uint16_t enc_left();
uint16_t enc_right();

void motor_set_power_left(int16_t power);
void motor_set_power_right(int16_t power);
void led_set(uint8_t on);
void esc_set_power(int16_t power);

void serial_init();
void tx_data(const void* ptr, uint8_t sz);
#define TX_VAR(v) tx_data(&v, sizeof(v))
uint8_t rx_byte_available();
uint8_t rx_data(void* ptr, uint8_t sz);
uint8_t rx_data_blocking(void* ptr, uint8_t sz);
#define RX_VAR(v) rx_data(&v, sizeof(v))
#define RX_VAR_BLOCKING(v) rx_data_blocking(&v, sizeof(v))
void rx_flush();

void lcd_init();
void lcd_clear();
void lcd_write_chars(uint8_t r, uint8_t c, const char *data, uint8_t size);
#define LCD_WRITE_STR(r,c,str) lcd_write_chars((r),(c),(str),strlen(str))
void lcd_write_int16(uint8_t r, uint8_t c, int16_t value);

void twi_init();
uint8_t twi_cmd_ready(uint8_t command);
uint8_t twi_read(uint8_t address, volatile void* read_dest, uint8_t size);
uint8_t twi_write(uint8_t address, const void* write_dest, uint8_t size);

typedef struct
{
	uint16_t left_kp, left_ki, left_kd;    // 8.8
	uint16_t right_kp, right_ki, right_kd; // 8.8
	uint8_t enc_frames, recv_samples;
	uint8_t left_reverse, right_reverse, esc_reverse;
	uint8_t esc_calibration_mode;
} config_struct;
#define num_cfgs 12

void config_init();
void config_status();
config_struct* get_config();




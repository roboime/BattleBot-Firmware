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
#include "binaries.h"

#define EXECUTE_ENC 1
#define RECV_AVAL0 2
#define RECV_AVAL1 4
#define RECV_AVAL2 8
#define EXECUTE_RECV (RECV_AVAL0|RECV_AVAL1|RECV_AVAL2)
#define flags GPIOR0

// A gente usa a funçaão wdt_off() porque a wdt_disable() possui erros
void wdt_off();

void input_init();
void input_read_enc();
void input_read_recv();

int16_t recv_get_ch(uint8_t ch);
uint16_t enc_left();
uint16_t enc_right();

void motor_set_power_left(int16_t power);
void motor_set_power_right(int16_t power);
void led_set(uint8_t on);

void serial_init();
void tx_data(const void* ptr, uint8_t sz);
#define TX_VAR(v) tx_data(&v, sizeof(v))
uint8_t rx_byte_available();
uint8_t rx_data(void* ptr, uint8_t sz);
uint8_t rx_data_blocking(void* ptr, uint8_t sz);
#define RX_VAR(v) rx_data(&v, sizeof(v))
#define RX_VAR_BLOCKING(v) rx_data_blocking(&v, sizeof(v))

typedef struct
{
	uint32_t left_kp, left_ki, left_kd;
	uint32_t right_kp, right_ki, right_kd;
	uint8_t left_blend, right_blend;
	uint8_t enc_frames, recv_samples;
} config_struct;
#define num_cfgs 10

void config_init();
void config_status();
config_struct* get_config();




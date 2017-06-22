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
#define RECV_AVAL3 16
#define EXECUTE_RECV (RECV_AVAL0|RECV_AVAL1|RECV_AVAL2|RECV_AVAL3)
#define flags GPIOR0

// A gente usa a funçaão wdt_off() porque a wdt_disable() possui erros
void wdt_off();

void input_init();
void input_read_enc();
void input_read_recv();

int16_t recv_get_ch(uint8_t ch);
uint16_t enc_value();

void motor_set_power(int16_t power);
void led_set(uint8_t on);

void serial_init();
void bluetooth_init();

void tx_data(const void* ptr, uint8_t sz);
#define TX_VAR(v) tx_data(&v, sizeof(v))
#define TX_STRING(s) tx_data(s, strlen(s))

uint8_t rx_byte_available();
uint8_t rx_data(void* ptr, uint8_t sz);
uint8_t rx_data_blocking(void* ptr, uint8_t sz);
#define RX_VAR(v) rx_data(&v, sizeof(v))
#define RX_VAR_BLOCKING(v) rx_data_blocking(&v, sizeof(v))

typedef struct
{
	uint16_t kp, ki, kd;    // 8.8
	uint8_t pid_blend;
	uint8_t enc_frames, recv_samples;
	uint8_t right_board;
} config_struct;
#define num_cfgs 7

void config_init();
void config_status();
config_struct* get_config();




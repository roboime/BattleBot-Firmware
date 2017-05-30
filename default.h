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
#define EXECUTE_RECV 2
extern volatile uint8_t flags;

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

typedef enum { left_kp, left_ki, left_kd, left_blend,
	right_kp, right_ki, right_kd, right_blend,
	enc_frames, recv_frames, num_cfgs } cfg_id;
void config_init();
void config_status();
int16_t get_config(cfg_id id);




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

// É bom que esteja em um registrador, porque fica mais rápido
//register volatile unsigned char curl0 asm("r3");
//register volatile unsigned char curl1 asm("r4");
//register volatile unsigned char curr0 asm("r5");
//register volatile unsigned char curr1 asm("r6");
//register volatile unsigned char overflow_count asm("r7");

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



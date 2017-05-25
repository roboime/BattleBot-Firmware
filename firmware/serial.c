//
// main.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo possui primitivas para a comunicação
// serial, os interrupts
//

#include "default.h"

// define um baud rate de 10000 bits por segundo
#define USART_BAUDRATE 8000UL
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#define QUEUE_SIZE 128
volatile uint8_t serial_queue[QUEUE_SIZE];
volatile uint8_t cur_read, cur_write;

// ISR para servir os bytes ao UDRE
//ISR (USART_UDRE_vect)
//{
//	UDR0 = serial_queue[cur_read];
//	cur_read = (cur_read + 1) % QUEUE_SIZE;
//	if (cur_read == cur_write) UCSR0B &= ~_BV(UDRIE0);
//}

// Função para inicializar o hardware USART para a
// comunicação via RX/TX
void serial_init()
{
	// Habilita a porta serial para transmissão, apenas,
	// interrupt quando o byte estiver pronto para transmissão
	UCSR0B = _BV(TXEN0);
	
	// 8-bits, sem bit de paridade
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	
	UBRR0H  = (BAUD_PRESCALE >> 8);
 	UBRR0L  = BAUD_PRESCALE;
 	
 	for (uint8_t i = 0; i < QUEUE_SIZE; i++)
 		serial_queue[i] = 0;
 	cur_read = cur_write = 0;
}

void tx_data(const void* ptr, uint8_t sz)
{
	const uint8_t* cptr = (const uint8_t*)ptr;
	
	for (uint8_t i = 0; i < sz; i++)
	{
		while( ( UCSR0A & ( 1 << UDRE0 ) ) == 0 ){}
		UDR0 = *cptr++;
	}
}



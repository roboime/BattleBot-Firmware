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

// define um baud rate de 19200 bits por segundo
#define UART_BAUD_RATE 19200UL
#define BAUD_PRESCALE ((F_CPU + UART_BAUD_RATE * 8L) / (UART_BAUD_RATE * 16L) - 1)

// Buffers circulares para facilitar o RX/TX por interrups
// cur_*_front aponta para a FRENTE da fila (de onde são tirados elementos)
// cur_*_back aponta para TRÁS da fila (onde são inseridos elementos)

#define QUEUE_SIZE_RX 128
volatile uint8_t serial_rx_queue[QUEUE_SIZE_RX];
volatile uint8_t cur_rx_front, cur_rx_back, last_rx;

#define QUEUE_SIZE_TX 128
volatile uint8_t serial_tx_queue[QUEUE_SIZE_TX];
volatile uint8_t cur_tx_front, cur_tx_back;

#define INCMOD(v,s) do { uint8_t k = v; if (++k == (s)) k = 0; v = k; } while (0)

// ISR para transmitir os bytes
ISR (USART_UDRE_vect)
{
	uint8_t to_transmit = serial_tx_queue[cur_tx_front];
	INCMOD(cur_tx_front, QUEUE_SIZE_TX);
	UDR0 = to_transmit;
	if (cur_tx_front == cur_tx_back) UCSR0B &= ~_BV(UDRIE0);
}

// ISR para receber os bytes
ISR (USART_RX_vect)
{
	uint8_t received = UDR0; // Necessário, o RX tem que ler o UDR0 para não ocorrer a interrupt de novo
	if (cur_rx_back == cur_rx_front) return;
	serial_rx_queue[cur_rx_back] = received;
	INCMOD(cur_rx_back, QUEUE_SIZE_RX);
	last_rx = 1;
}

// Função para inicializar o hardware USART para a
// comunicação via RX/TX
void serial_init()
{

	// 8 bits, com bit de paridade par
	UCSR0C = _BV(UPM01) | _BV(USBS0) | _BV(UCSZ01) | _BV(UCSZ00);
	UBRR0 = BAUD_PRESCALE;

	// Habilita a porta serial para transmissão e recepção
	// interrupt quando for recebido um byte
	UCSR0B = _BV(TXEN0) | _BV(RXEN0)/* | _BV(RXCIE0)*/;
 	
 	for (uint8_t i = 0; i < QUEUE_SIZE_TX; i++)
 		serial_tx_queue[i] = 0;
  	for (uint8_t i = 0; i < QUEUE_SIZE_RX; i++)
 		serial_rx_queue[i] = 0;
 	cur_tx_front = cur_tx_back = 0;
 	cur_rx_front = cur_rx_back = 0;
 	last_rx = 0;
}

void tx_data(const void* ptr, uint8_t sz)
{
	UCSR0B &= ~_BV(UDRIE0);

	const uint8_t* cptr = (const uint8_t*)ptr;
	
	for (uint8_t i = 0; i < sz; i++)
	{
		serial_tx_queue[cur_tx_back] = cptr[i];
		INCMOD(cur_tx_back, QUEUE_SIZE_TX);
		if (cur_tx_back == cur_tx_front) break;
	}
	
	UCSR0B |= _BV(UDRIE0);
}

uint8_t rx_bytes_available()
{
	if (cur_rx_back == cur_rx_front)
		return last_rx ? QUEUE_SIZE_RX : 0;
	else if (cur_rx_back < cur_rx_front)
		return cur_rx_front - cur_rx_back;
	else return QUEUE_SIZE_RX - cur_rx_back + cur_rx_front;
}

uint8_t rx_data(void* ptr, uint8_t sz)
{
	if (rx_bytes_available() < sz) return 0;
	
	uint8_t* cptr = (uint8_t*)ptr;
	
	for (uint8_t i = 0; i < sz; i++)
	{
		cptr[i] = serial_rx_queue[cur_rx_front];
		INCMOD(cur_rx_front, QUEUE_SIZE_RX);
		last_rx = 0;
	}
	
	return 1;
}



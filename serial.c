//
// serial.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo possui primitivas para a comunicação serial
//

#include "default.h"

// define um baud rate de 19200 bits por segundo
#define UART_BAUD_RATE 19200UL
#define BAUD_PRESCALE ((F_CPU + UART_BAUD_RATE * 8L) / (UART_BAUD_RATE * 16L) - 1)

#define RX_TIMEOUT 5600

// Função para inicializar o hardware USART para a
// comunicação via RX/TX
void serial_init()
{

	// 8 bits, com bit de paridade par
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UBRR0 = BAUD_PRESCALE;

	// Habilita a porta serial para transmissão e recepção
	// interrupt quando for recebido um byte
	UCSR0B = _BV(TXEN0) | _BV(RXEN0);
}

void tx_data(const void* ptr, uint8_t sz)
{
	const uint8_t* cptr = (const uint8_t*)ptr;
	
	for (uint8_t i = 0; i < sz; i++)
	{
		while (!(UCSR0A & _BV(UDRE0)));
		UDR0 = cptr[i];
	}

}

uint8_t rx_byte_available()
{
	return (UCSR0A & _BV(RXC0)) != 0;
}

uint8_t rx_data(void* ptr, uint8_t sz)
{
	if (!(UCSR0A & _BV(RXC0))) return 0;
	
	uint8_t* cptr = (uint8_t*)ptr;
	
	for (uint8_t i = 0; i < sz; i++)
	{
		uint16_t timeout = RX_TIMEOUT;
		while (!(UCSR0A & _BV(RXC0)))
			if (timeout-- == 0) return 0;
		cptr[i] = UDR0;
	}
	
	return 1;
}

uint8_t rx_data_blocking(void* ptr, uint8_t sz)
{
	while (!(UCSR0A & _BV(RXC0)));
	return rx_data(ptr, sz);
}


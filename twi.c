//
// twi.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo possui primitivas para a comunicação via TWI
// (o protocolo utilizado pelo sensor de corrente)
//

#include "default.h"

#define INCMOD(v,s) do { uint8_t k = v; if (++k == (s)) k = 0; v = k; } while(0)
#define F_SCL 100000

#define TWIDF (_BV(TWEN) | _BV(TWIE))

typedef struct
{
	union { struct { uint8_t read:1; uint8_t address:7; }; uint8_t sla_rw; };
	uint8_t size;
	union { const char* write; volatile char* read; } buffer;
} twi_msg;

#define TWI_QUEUE_SIZE 8
static volatile uint8_t cmd_ready[8];
static volatile twi_msg twi_queue[TWI_QUEUE_SIZE];
// twi_back aponta para TRÁS da fila (onde são inseridos elementos)
// twi_front aponta para a FRENTE da fila (de onde são retirados elementos)
static volatile uint8_t twi_back = 0, twi_front = 0, last_twi = 0;

static void twi_new_msg()
{
	// Seta a flag de comando pronto e incrementa o twi_back
	cmd_ready[twi_front] = 1;
	INCMOD(twi_front, TWI_QUEUE_SIZE);
	if (twi_front == twi_back) // fila vazia, envia comando de parar
	{
		TWCR |= _BV(TWSTO);
		TWCR |= _BV(TWINT);
	}
	else // ainda há comandos a serem executados, comando de reiniciar
	{
		TWCR |= _BV(TWSTA);
		TWCR |= _BV(TWINT);
		flags |= TWI_PREV_START;
	}
	last_twi = 0; // Operação de leitura
}

// ISR para controle do TWI
ISR (TWI_vect)
{
	if (flags & TWI_PREV_START)
	{
		flags &= ~TWI_PREV_START;
		TWCR &= ~_BV(TWSTA);
		
		if (TWSR & 0x18) // condição START ou REPEATED START
		{
			// Aqui só é válido se há alguém na fila
			const twi_msg* cur_msg = &twi_queue[twi_front];
			TWDR = cur_msg->sla_rw;
			
			if (cur_msg->read && cur_msg->size > 1) TWCR |= _BV(TWEA);
			else TWCR &= ~_BV(TWEA);
			TWCR |= _BV(TWINT);
		}
		else twi_new_msg();
	}
	else
	{
		// Aqui também só é válido se há alguém na fila
		twi_msg* cur_msg = &twi_queue[twi_front];
		
		if (cur_msg->read) // Se o comando for de letura
		{
			uint8_t status = TWSR & 0xF8;
			
			// Recepção de byte
			if (status == 0x50 || status == 0x58)
			{
				*cur_msg->buffer.read = TWDR;
				cur_msg->buffer.read++;
				cur_msg->size--;
			}
			
			// Mensagem de status
			// casos: SLA+R enviado e ACK recebido
			//        byte recebido e ACK retornado
			// enviar comando de leitura e enviar ACK (ou não)
			if (status == 0x40 || status == 0x50)
			{
				if (cur_msg->size > 1) TWCR |= _BV(TWEA);
				else TWCR &= ~_BV(TWEA);
				TWCR |= _BV(TWINT);
			}
			// Mensagem de status
			// casos: SLA+R enviado e nenhum ACK recebido
			//        último byte recebido e NACK retornado
			// proceder nova mensagem
			else twi_new_msg();
		}
		else // Se o comando for de escrita
		{
			uint8_t status = TWSR & 0xF8;
			
			// Se ACK for recebido, podemos continuar escrevendo
			// desde que ainda haja 
			if ((status == 0x18 || status == 0x28) && cur_msg->size > 0)
			{
				TWDR = *cur_msg->buffer.write;
				cur_msg->buffer.write++;
				cur_msg->size--;
				TWCR |= _BV(TWINT);
			}
			// Senão, paramos de escrever
			else twi_new_msg();
		}
	}
}

void twi_init()
{
	TWBR = ((F_CPU/F_SCL) - 16)/2; // Configuração da frequência
	TWCR = TWIDF; // habilita TWI e interrupts para o TWI
	TWSR = 0;     // prescaler de 0
}

uint8_t twi_cmd_ready(uint8_t command)
{
	return cmd_ready[command];
}

uint8_t twi_read(uint8_t address, volatile void* read_dest, uint8_t size)
{
	if (last_twi == 1 && twi_front == twi_back) return -1;
	
	TWCR &= ~_BV(TWIE);
	
	cmd_ready[twi_back] = 0;
	
	twi_msg* cur_msg = &twi_queue[twi_back];
	cur_msg->size = size;
	cur_msg->address = address;
	cur_msg->read = 1;
	cur_msg->buffer.read = (volatile char*)read_dest;
	
	uint8_t old_twi_back = twi_back;
	INCMOD(twi_back, TWI_QUEUE_SIZE);
	
	if (old_twi_back == twi_front)
	{
		TWCR &= ~_BV(TWSTO);
		TWCR |= _BV(TWSTA);
		flags |= TWI_PREV_START;
	}
	
	last_twi = 1;
	TWCR |= _BV(TWIE);
	
	return old_twi_back;
} 

uint8_t twi_write(uint8_t address, const void* write_dest, uint8_t size)
{
	if (last_twi == 1 && twi_front == twi_back) return -1;
	
	TWCR &= ~_BV(TWIE);
	
	cmd_ready[twi_back] = 0;
	
	twi_msg* cur_msg = &twi_queue[twi_back];
	cur_msg->size = size;
	cur_msg->address = address;
	cur_msg->read = 0;
	cur_msg->buffer.write = (const char*)write_dest;
	
	uint8_t old_twi_back = twi_back;
	INCMOD(twi_back, TWI_QUEUE_SIZE);
	
	if (old_twi_back == twi_front)
	{
		TWCR &= ~_BV(TWSTO);
		TWCR |= _BV(TWSTA);
		flags |= TWI_PREV_START;
	}
	
	last_twi = 1;
	TWCR |= _BV(TWIE);
	
	return old_twi_back;
}



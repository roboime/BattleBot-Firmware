//
// config.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo contém as funções que compõem o modo
// de configuração
//

#include "default.h"

#define ACK 0xAC
#define ERROR_INVALID_COMMAND 0xE0
#define ERROR_INVALID_VARIABLE 0xE1
#define ERROR_INVALID_PARAMETERS 0xE2
#define ERROR_INVALID_VALUE 0xE3
#define ERROR_BUFFER_TOO_LONG 0xE4

#define READ_CHUNK 0x00
#define WRITE_CHUNK 0x30
#define FINISH_CMD 0xFF

#define MAX_BUFFER_LENGTH 8

#define TX_ERROR(code) do { uint8_t err = (code); TX_VAR(err); continue; } while (0)
#define TX_ACK() do { uint8_t ack = ACK; TX_VAR(ack); } while (0)

static int16_t config_vars[num_cfgs];

inline static uint8_t valid_cfg(uint8_t id, int16_t var)
{
	switch (id)
	{
		case left_blend: case right_blend: return var >= 0 && var < 256;
		case enc_frames: return var >= 0 && var < 32;
		case recv_frames: return var >= 0 && var < 32 && var % 2 == 1;
		default: return 1;
	}
} 

void config_init()
{
	
}

void config_status()
{
	// Aqui a gente não precisa de interrupt
	cli();

	// Lê as configurações
	for (;;)
	{
		wdt_reset();
		uint8_t size;
		if (!RX_VAR(size)) continue;
		if (size > MAX_BUFFER_LENGTH)
			TX_ERROR(ERROR_BUFFER_TOO_LONG);

		// Um buffer vazio não é um comando válido
		if (size == 0) TX_ERROR(ERROR_INVALID_COMMAND);

		// Se o computador demorar muito para mandar o
		// frame, descartar
		uint8_t buffer[MAX_BUFFER_LENGTH];
		if (!rx_data_blocking(buffer, size)) continue;
		
		// Comando de leitura
		if ((buffer[0] & 0xF0) == READ_CHUNK)
		{
			uint8_t cfg = buffer[0] & 0x0F;
			if (cfg >= num_cfgs)
				TX_ERROR(ERROR_INVALID_VARIABLE);
			TX_ACK();
			TX_VAR(config_vars[cfg]);
		}
		// Comando de escrita
		else if ((buffer[0] & 0xF0) == WRITE_CHUNK)
		{
			if (size < sizeof(uint8_t) + sizeof(int16_t))
				TX_ERROR(ERROR_INVALID_PARAMETERS);
			
			uint8_t cfg = buffer[0] & 0x0F;
			if (cfg >= num_cfgs)
				TX_ERROR(ERROR_INVALID_VARIABLE);
			
			int16_t* new_param = (int16_t*)(&buffer[1]);
			if (!valid_cfg(cfg, *new_param))
				TX_ERROR(ERROR_INVALID_VALUE);
				
			config_vars[cfg] = *new_param;
			TX_ACK();
		}
		// Comando de finalizar escrita e reiniciar processador
		else if (buffer[0] == FINISH_CMD)
		{
			TX_ACK();
			break;
		}
		// Comando inválido
		else TX_ERROR(ERROR_INVALID_COMMAND);
	}

	// Loop infinito para forçar o processador a resetar (watchdog)
	for (;;);
}

int16_t get_config(cfg_id id)
{
	if (id >= num_cfgs) return 0;
	return config_vars[id];
}

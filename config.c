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
#include <avr/eeprom.h>

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

int16_t eeprom_configs[3][num_cfgs] EEMEM;
int16_t eeprom_check[3] EEMEM;

static int16_t config_vars[num_cfgs];

// Checksum = 1*i1 + 2*i2 + 3*i3 + ... + n*in;
inline static int16_t check_fun(int16_t values[num_cfgs])
{
	int16_t res = 0;
	for (uint8_t i = 0; i < num_cfgs; i++)
		res ^= (i+1) * values[i];
	return res;
}

void config_init()
{
	// Lê da EEPROM três vezes, para minimizar o risco de leitura errada
	int16_t config_copy[3][num_cfgs];
	int16_t check[3];
	
	eeprom_read_block(config_copy, eeprom_configs, 3*num_cfgs*sizeof(int16_t));
	eeprom_read_block(check, eeprom_check, 3*sizeof(int16_t));
	
	// Checksum simples, só para ver se deu alguma coisa
	for (uint8_t j = 0; j < 3; j++)
		if (check[j] != check_fun(config_copy[j]))
		{
			for (uint8_t i = 0; i < num_cfgs; i++) config_copy[j][i] = 0;
			check[j] = 0;
		}
		
	for (uint8_t i = 0; i < num_cfgs; i++)
	{
		uint8_t v0 = config_copy[0][i];
		uint8_t v1 = config_copy[1][i];
		uint8_t v2 = config_copy[2][i];
		
		if (v0 == v1 || v0 == v2) config_vars[i] = v0;
		else if (v1 == v2) config_vars[i] = v1;
		else config_vars[i] = 0;
	}
}

inline static void config_save()
{
	// Checksum
	int16_t check[3];
	check[0] = check[1] = check[2] = check_fun(config_vars);
	
	// Escreve três vezes para haver baixo risco de corrupção de dados
	eeprom_update_block(check, eeprom_check, 3*sizeof(int16_t));
	
	for (uint8_t i = 0; i < 3; i++)
		eeprom_update_block(config_vars, eeprom_configs[i], num_cfgs*sizeof(int16_t));
}

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

void config_status()
{
	// Aqui a gente não precisa de interrupt
	cli();

	// Lê as configurações
	for (;;)
	{
		led_set(1);
		wdt_reset();
		uint8_t size;
		if (!RX_VAR(size)) continue;
		if (size > MAX_BUFFER_LENGTH)
			TX_ERROR(ERROR_BUFFER_TOO_LONG);

		// Um buffer vazio não é um comando válido
		if (size == 0) TX_ERROR(ERROR_INVALID_COMMAND);

		// Se o computador demorar muito para mandar o
		// frame, descartar
		led_set(0);
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

	// Salva os dados na EEPROM
	config_save();

	// Loop infinito para forçar o processador a resetar (watchdog)
	led_set(1);
	for (;;);
}

int16_t get_config(cfg_id id)
{
	if (id >= num_cfgs) return 0;
	return config_vars[id];
}

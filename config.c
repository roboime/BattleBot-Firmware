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
#include <avr/pgmspace.h>

#define ACK 0xAC
#define ERROR_INVALID_COMMAND 0xE0
#define ERROR_INVALID_VARIABLE 0xE1
#define ERROR_INVALID_PARAMETERS 0xE2
#define ERROR_INVALID_VALUE 0xE3
#define ERROR_BUFFER_TOO_LONG 0xE4
#define ERROR_TIMEOUT 0xE5

#define READ_CHUNK 0x00
#define WRITE_CHUNK 0x30
#define FINISH_CMD 0xFF

#define MAX_BUFFER_LENGTH 8

#define EEMEM __attribute__((section(".eeprom")))

// Força o endereço 0 a não ser utilizado (ATMEL não recomenda)
uint8_t EEMEM force_offset[4] __attribute__((used));
config_struct EEMEM eeprom_configs[3];
uint8_t EEMEM eeprom_check[3];

static config_struct configs;

const config_struct PROGMEM default_config = { 0x0800, 0x0000, 0x0000, 255, 8, 5, 0 };

// Funções para leitura e escrita de EEPROM
void read_eeprom(void* dst, const void* src, uint8_t sz)
{
	uint8_t* cdst = (uint8_t*)dst;
	const uint8_t* csrc = (const uint8_t*)src;

	for (uint8_t i = 0; i < sz; i++)
	{
		wdt_reset();
		while (EECR & _BV(EEPE));   // Espera o bit EEPE ir a 0
		EEAR = (uintptr_t)(csrc+i); // Define o endereço de leitura
		EECR |= _BV(EERE);          // Comanda a leitura da EEPROM
		cdst[i] = EEDR;             // Guarda o byte lido na SRAM
	}
}

void update_eeprom(void* dst, const void* src, uint8_t sz)
{
	uint8_t* cdst = (uint8_t*)dst;
	const uint8_t* csrc = (const uint8_t*)src;

	for (uint8_t i = 0; i < sz; i++)
	{
		wdt_reset();
		while (EECR & _BV(EEPE));   // Espera o bit EEPE ir a 0
		EEAR = (uintptr_t)cdst+i;   // Endereço de escrita
		
		EECR |= _BV(EERE);          // Byte para comparação
		uint8_t byte = EEDR;
		
		if (csrc[i] != byte)
		{
			EEAR = (uintptr_t)cdst+i;
			EEDR = csrc[i];           // Byte a ser escrito
			EECR |= _BV(EEMPE);       // "Desativa" a proteção da escrita
			EECR |= _BV(EEPE);        // Comanda a escrita
		}
	}
}

// memcpy
void* memcpy(void* dst, const void* src, size_t size);

// Checksum = 1*i1 ^ 2*i2 ^ ... ^ n*in;
inline static uint8_t check_fun(const config_struct *cfg)
{
	int16_t res = 0;
	const uint8_t* values = (const uint8_t*)cfg;
	for (uint8_t i = 0; i < sizeof(cfg); i++)
		res ^= (i+1) * values[i];
	return res;
}


void config_init()
{
	config_struct config_copy[3];
	uint8_t check[3];
	
	// Lê da EEPROM três vezes, para minimizar o risco de leitura errada
	read_eeprom(config_copy, eeprom_configs, sizeof(eeprom_configs));
	read_eeprom(check, eeprom_check, sizeof(eeprom_check));
	
	// Checksum simples, só para ver se a leitura completou
	for (uint8_t j = 0; j < 3; j++)
		if (check[j] != check_fun(&config_copy[j]))
		{
			memcpy_P(&config_copy[j], &default_config, sizeof(config_struct));
			check[j] = 0;
		}
		
	// "Votação": se dois valores concordarem, esse será o valor utilizado
#define VOTE_PARAM(par) do                      \
{                                               \
	typeof(configs.par) v0, v1, v2;             \
	v0 = config_copy[0].par;                    \
	v1 = config_copy[1].par;                    \
	v2 = config_copy[2].par;                    \
	if (v0 == v1 || v0 == v2) configs.par = v0; \
	else if (v1 == v2) configs.par = v1;        \
	else configs.par = 0;                       \
} while (0)
	
	VOTE_PARAM(kp);
	VOTE_PARAM(ki);
	VOTE_PARAM(kd);
	VOTE_PARAM(pid_blend);
	VOTE_PARAM(enc_frames);
	VOTE_PARAM(recv_samples);
	VOTE_PARAM(right_board);
	
#undef VOTE_PARAM
}

inline static void config_save()
{
	// Checksum
	int16_t check[3];
	check[0] = check[1] = check[2] = check_fun(&configs);
	
	// Escreve três vezes para haver baixo risco de corrupção de dados
	update_eeprom(eeprom_check, check, sizeof(eeprom_check));
	
	for (uint8_t i = 0; i < 3; i++)
		update_eeprom(&eeprom_configs[i], &configs, sizeof(config_struct));
		
	// Aguarda o EEPROM terminar seu serviço
	while (EECR & _BV(EEPE));
}

// Tamanho de cada elemento na struct de configuração (bem que podia ser gerado automaticamente :/)
inline static uint8_t cfg_size(uint8_t id)
{
	switch (id)
	{
		case 0: return sizeof(configs.kp);
		case 1: return sizeof(configs.ki);
		case 2: return sizeof(configs.kd);
		case 3: return sizeof(configs.pid_blend);
		case 4: return sizeof(configs.enc_frames);
		case 5: return sizeof(configs.recv_samples);
		case 6: return sizeof(configs.right_board);
		default: return 0;
	}
}

// Ponteiro para cada elemento na struct de configuração
inline static void* cfg_ptr(uint8_t id)
{
	switch (id)
	{
		case 0: return &configs.kp;
		case 1: return &configs.ki;
		case 2: return &configs.kd;
		case 3: return &configs.pid_blend;
		case 4: return &configs.enc_frames;
		case 5: return &configs.recv_samples;
		case 6: return &configs.right_board;
		default: return 0;
	}
}

void config_status() __attribute__((noreturn));
void config_status()
{
	// Aqui a gente não precisa de interrupt
	cli();
	{ uint8_t ack = ACK; TX_VAR(ack); }

	led_set(1);

	// Lê as configurações
	for (;;)
	{
	reinit:
#define TX_ERROR(code) do { TX_VAR(sz); uint8_t err = (code); TX_VAR(err); goto reinit; } while (0)
#define TX_ACK() do { TX_VAR(sz); uint8_t ack = ACK; TX_VAR(ack); } while (0)
		wdt_reset();
		uint8_t sz = 1;
		uint8_t size;
		
		if (!RX_VAR(size)) goto reinit;
		if (size > MAX_BUFFER_LENGTH)
		{
			// Descarrega o buffer para o próximo comando
			uint8_t dummy;
			while (size--) RX_VAR_BLOCKING(dummy);
		
			TX_ERROR(ERROR_BUFFER_TOO_LONG);
		}

		// Um buffer vazio não é um comando válido
		if (size == 0) TX_ERROR(ERROR_INVALID_COMMAND);

		// Se o computador demorar muito para mandar o
		// frame, descartar
		uint8_t buffer[MAX_BUFFER_LENGTH];
		if (!rx_data_blocking(buffer, size))
			TX_ERROR(ERROR_TIMEOUT);
		
		// Comando de leitura
		if ((buffer[0] & 0xF0) == READ_CHUNK)
		{
			uint8_t cfg = buffer[0] & 0x0F;
			if (cfg >= num_cfgs)
				TX_ERROR(ERROR_INVALID_VARIABLE);
				
			sz = 1 + cfg_size(cfg);
			TX_ACK();
			tx_data(cfg_ptr(cfg), cfg_size(cfg));
		}
		// Comando de escrita
		else if ((buffer[0] & 0xF0) == WRITE_CHUNK)
		{
			uint8_t cfg = buffer[0] & 0x0F;
		
			if (cfg >= num_cfgs)
				TX_ERROR(ERROR_INVALID_VARIABLE);
			if (size < sizeof(uint8_t) + cfg_size(cfg))
				TX_ERROR(ERROR_INVALID_PARAMETERS);
			
			memcpy(cfg_ptr(cfg), &buffer[1], cfg_size(cfg));
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
#undef TX_ACK
#undef TX_ERROR
	}

	// Salva os dados na EEPROM
	config_save();

	// Loop infinito para forçar o processador a resetar (watchdog)
	for (;;);
}

config_struct* get_config()
{
	return &configs;
}

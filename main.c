//
// main.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo tem a função main, que vai ser a base para
// todas as outras funções rodarem, além de ser a base da
// configuração de inicialização do microcontrolador.
// A descrição do que cada pino do ATMega328p faz no programa
// pode ser encontrada em wiring.txt
//
// Aqui há um uso pesado de aritmética fixed-point. Por isso, há
// anotações em inteiros que são interpretados dessa forma
//

#include "default.h"

#define BLINK_FRAMES 8
#define HANDSHAKE_RX_BYTE 0x55

#define BLUETOOTH_INIT 0

#define SGN(x,v) ((x)>0 ? (v) : (x)<0 ? -(v) : 0)

#include <stdlib.h>

// Isso aqui tem que ser executado o mais rápido possível (antes do main)
void pre_main() __attribute__((naked,used,section(".init3")));
void pre_main() { wdt_off(); }

// Variáveis para o PID: todas elas são fixed-point 16.16
int32_t cur_out = 0, err_int = 0, last_err = 0, target = 0;

void main() __attribute__((noreturn));
void main()
{
	// Desabilitar interrupts
	cli();

	// Configuração das direções das portas:
	DDRB = B00000000; // grupo B: nada
	DDRC = B00000001; // grupo C: LED de apoio, receptor
	DDRD = B01100000; // grupo D: motor, encoder, RX/TX
	
	// Configuração dos interrupts de mudança de pino
	PCMSK1 = B00001111; // grupo C: receptor
	PCICR = B010; // habilita os três interrupts

	// Configuração dos interrupts externos para a leitura dos encoders
	EICRA = B1111; // interrupt na subida lógica de cada um deles
	EIMSK = B11;   // habilita os dois interrupts externos

	// Configuração dos timers: Timer0 usado no motor esquerdo, Timer1 usado no motor direito
	TCCR0A = B10100011; // Timer0: as duas saídas invertidas (LOW e depois HIGH)
	TCCR0B = B00000011; // Timer0: prescaler de 8 ciclos, fast PWM
	TIMSK0 = B00000001; // Timer0: habilitar interrupt no overflow, usado para contar ciclos
	OCR0A = 0;
	OCR0B = 0;  // Timer0: PWM de 0 nas duas saídas

	// Desativar alguns periféricos para redução de consumo de energia
	PRR = B11001101;

	// Zera todos os dados usados pelos módulos de input, output, serial e config
	serial_init();
	config_init();
	input_init();
	flags = 0;

	// Configuração do timer de watchdog, para resetar o microprocessador caso haja alguma falha
	wdt_enable(WDTO_60MS);

	uint32_t counter = 250000;
	while (counter--)
	{	
		wdt_reset();
		// Detecta o handshake para o modo de configuração
		uint8_t rx;
		while (RX_VAR(rx))
			if (rx == HANDSHAKE_RX_BYTE)
			{
				DDRB = 0;
				DDRD = 0;
				config_status();
			}
	}

	// Habilita interrupts de novo
	sei();
	
	// Loop infinito
	for(;;)
	{
		// Loop principal
		if (flags & EXECUTE_ENC)
		{
			static uint8_t frame_counter = 0;

			flags &= (uint8_t)~EXECUTE_ENC;
			input_read_enc();

			led_set(frame_counter < BLINK_FRAMES);
			if (++frame_counter == 2*BLINK_FRAMES) frame_counter = 0;
			
			// Controle do PID: enc_value() 16.0
			int32_t enc = (int32_t)SGN(cur_out, enc_value()) << 16;   // enc 16.16
			
			// PID
			int32_t kp = get_config()->kp, ki = get_config()->ki, kd = get_config()->kd;
			            
			int32_t err = target - enc;                             // 16.16
			err_int += (int32_t)ki * (err >> 8);                    // 16.16
			cur_out = (int32_t)kp * (err >> 8) + err_int;           // 16.16
			//*last_err = err;                                      // 16.16

			int32_t knob_blend = recv_get_ch(3) + 256;
			if (knob_blend < 0) knob_blend = 0;
			if (knob_blend > 512) knob_blend = 512;
			
			// os dois aqui são 16.16
			//    16.16    16.16                                      0.8        16.16    16.16
			int32_t out = target + knob_blend * ((get_config()->pid_blend  * ((cur_out - target) >> 8)) / 512);
			
			// Finalmente
			motor_set_power(out >> 16);

			// Detecta o handshake para o modo de configuração
			uint8_t rx;
			while (RX_VAR(rx))
				if (rx == HANDSHAKE_RX_BYTE)
				{
					DDRB = 0;
					DDRD = 0;
					config_status();
				}
		}
		if (flags & EXECUTE_RECV)
		{
			input_read_recv();
			
			int16_t ch0 = recv_get_ch(0);       // 16.0
			int16_t ch1 = recv_get_ch(1);       // 16.0
			if (recv_get_ch(2) > 0) ch0 = -ch0;
			
			target = (int32_t)(ch1 - ch0) << 16;  // 16.16
		}
		
		// Coloca o uC em modo de baixo consumo de energia
		sleep_mode();
	}
}

// Essa função foi escrita porque a wdt_disable() original possui erros de operação
void wdt_off()
{
	uint8_t oldSREG = SREG;
	cli();

	wdt_reset();
	MCUSR = 0;
	WDTCSR |= _BV(WDCE) | _BV(WDE); // "Desativa" a proteção de leitura
	WDTCSR = 0;                     // Desliga o watchdog

	SREG = oldSREG;
}

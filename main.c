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

#include "default.h"

#define BLINK_FRAMES 8
#define HANDSHAKE_RX_BYTE 0x55

// Isso aqui tem que ser executado o mais rápido possível (antes do main)
void pre_main() __attribute__((naked,used,section(".init3")));
void pre_main() { wdt_off(); }

void main()
{
	// Desabilitar interrupts
	cli();

	// Configuração das direções das portas:
	DDRB = B00000110; // grupo B: motor direito, encoder direito
	DDRC = B00000001; // grupo C: LED de apoio, receptor
	DDRD = B01100000; // grupo D: motor esquerdo, encoder esquerdo, RX/TX
	
	// Configuração do estado inicial e resistores de pullup
	PORTB = B00000001; // grupo B: pullup na DIP switch
	PORTC = B00000010; // grupo C: pullup nos pinos desconectados
	PORTD = B10010000; // grupo D: pullup na DIP switch
	
	// Configuração dos interrupts de mudança de pino
	PCMSK1 = B00011100; // grupo C: receptor
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
	
	TCCR1A = B11110001; // Timer1: as duas saídas invertidas (LOW e depois HIGH)
	TCCR1B = B00001011; // Timer1: prescaler de 64 ciclos, fast PWM;
	TIMSK1 = B00000000; // Timer1: desabilitar todos os interrupts
	OCR1A = 0;
	OCR1B = 0;  // Timer1: PWM de 0 nas duas saídas
	
	TCCR2A = B00000000; // Timer2:
	TCCR2B = B00000000; // Timer2: completamente desabilitado
	TIMSK2 = B00000000; // Timer2:
	
	// Desativar alguns periféricos para redução de consumo de energia
	PRR = B11000101;

	// Zera todos os dados usados pelos módulos de input, output, serial e config
	serial_init();
	config_init();
	input_init();
	flags = 0;
	
	// Configuração do timer de watchdog, para resetar o microprocessador caso haja alguma falha
	wdt_enable(WDTO_60MS);

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
			
			// Detecta o handshake para o modo de configuração
			uint8_t rx;
			if (RX_VAR(rx) && rx == HANDSHAKE_RX_BYTE)
				config_status();
		}
		if (flags & EXECUTE_RECV)
		{
			input_read_recv();
			
			int16_t ch0 = recv_get_ch(0);
			int16_t ch1 = recv_get_ch(1);
			if (recv_get_ch(2) > 0) ch0 = -ch0;

			motor_set_power_left(ch1-ch0);
			motor_set_power_right(ch1+ch0);
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

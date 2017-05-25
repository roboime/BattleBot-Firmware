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

volatile uint8_t flags = 0;

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
	PORTC = B00100010; // grupo C: pullup nos pinos desconectados
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
	
	// Configuração do timer de watchdog, para resetar o microprocessador caso haja alguma falha
	wdt_enable(WDTO_60MS);

	// Zera todos os dados usados pelos módulos de input, output, serial e timing
	input_init();
	serial_init();
	flags = 0;
	
	// Habilita interrupts de novo
	sei();
	
	// Loop infinito
	for(;;)
	{
		// Loop principal
		if (flags | EXECUTE_ENC)
		{
			flags &= (uint8_t)~EXECUTE_ENC;
			input_read_enc();
			
			uint16_t encl = enc_left();
			uint16_t encr = enc_right();
			
			uint8_t strt = 0xCF;
			TX_VAR(strt);
			tx_data(&encl, sizeof(uint16_t));
			tx_data(&encr, sizeof(uint16_t));
		}
		if (flags | EXECUTE_RECV)
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
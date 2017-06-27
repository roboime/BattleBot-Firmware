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

#define SGN(x,v) ((x)>0 ? (v) : (x)<0 ? -(v) : 0)

#include <stdlib.h>

// Isso aqui tem que ser executado o mais rápido possível (antes do main)
void pre_main() __attribute__((naked,used,section(".init3")));
void pre_main() { wdt_off(); }

// Variáveis para o PID: todas elas são fixed-point 16.16
int32_t cur_out_l = 0, err_int_l = 0, last_err_l = 0, target_l = 0;
int32_t cur_out_r = 0, err_int_r = 0, last_err_r = 0, target_r = 0;

//                    16.16           16.16        4.12        4.12        4.12             16.16             16.16              16.16
void pid_control(int32_t in, int32_t target, int16_t kp, int16_t ki, int16_t kd, int32_t *cur_out, int32_t *err_int, int32_t *last_err);

void main() __attribute__((noreturn));
void main()
{
	// Desabilitar interrupts
	cli();

	// Configuração das direções das portas:
	DDRB = B00111111; // grupo B: motor direito, encoder direito
	DDRC = B00000000; // grupo C: LED de apoio, receptor
	DDRD = B11110000; // grupo D: motor esquerdo, encoder esquerdo, RX/TX
	
	// Configuração do estado inicial e resistores de pullup
	PORTB = B00000000; // grupo B: pullup na DIP switch
	PORTC = B00000000; // grupo C: pullup nos pinos desconectados
	PORTD = B00000000; // grupo D: pullup na DIP switch
	
	// Configuração dos interrupts de mudança de pino
	PCMSK1 = B00111100; // grupo C: receptor
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
	
	TCCR1A = B10100001; // Timer1: as duas saídas invertidas (LOW e depois HIGH)
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
	
	lcd_init();
	LCD_WRITE_STR(0,0,"LP:");
	LCD_WRITE_STR(1,0,"RP:");
	LCD_WRITE_STR(0,8,"LS:");
	LCD_WRITE_STR(1,8,"RS:");

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
			
			// Controle do PID: enc_left() e enc_right() 16.0
			int32_t enc_l = (int32_t)SGN(cur_out_l, enc_left()) << 16;   // enc_l 16.16
			int32_t enc_r = (int32_t)SGN(cur_out_r, enc_right()) << 16;  // enc_r 16.16
			
			// PID do motor esquerdo
			pid_control(enc_l, target_l,
			            get_config()->left_kp, get_config()->left_ki, get_config()->left_kd,
			            &cur_out_l, &err_int_l, &last_err_l);
			
			// PID do motor direito  
			pid_control(enc_r, target_r,
			            get_config()->right_kp, get_config()->right_ki, get_config()->right_kd,
			            &cur_out_r, &err_int_r, &last_err_r);

			// quarto canal			
			int32_t knob_blend = recv_get_ch(3) + 256;
			if (knob_blend < 0) knob_blend = 0;
			if (knob_blend > 512) knob_blend = 512;
			
			// os dois aqui são 16.16
			//      16.16      16.16                         0.8          16.16      16.16
			cur_out_l = target_l + knob_blend * ((get_config()->left_blend  * ((cur_out_l - target_l) >> 8)) / 512);
			cur_out_r = target_r + knob_blend * ((get_config()->right_blend * ((cur_out_r - target_r) >> 8)) / 512);
			
			CLAMP(cur_out_l, 250L << 16);
			CLAMP(cur_out_r, 250L << 16);
			
			// Finalmente
			motor_set_power_left(cur_out_l >> 16);  // de volta para 16.0
			motor_set_power_right(cur_out_r >> 16); // idem
			
			// Depuração via LCD
			lcd_write_int16(0, 3, cur_out_l >> 16);
			lcd_write_int16(1, 3, cur_out_r >> 16);
			
			lcd_write_chars(0, 11, " ", 1);
			lcd_write_chars(1, 11, " ", 1);
			lcd_write_int16(0,12, enc_l >> 16);
			lcd_write_int16(1,12, enc_r >> 16);
			
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
			
			target_l = (int32_t)(ch1 - ch0) << 16;  // 16.16
			target_r = (int32_t)(ch1 + ch0) << 16;  // 16.16
			
			CLAMP(target_l, 250L << 16);
			CLAMP(target_r, 250L << 16);
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

//                    16.16           16.16         8.8         8.8         8.8             16.16             16.16              16.16
void pid_control(int32_t in, int32_t target, int16_t kp, int16_t ki, int16_t kd, int32_t *cur_out, int32_t *err_int, int32_t *last_err)
{
	int32_t err = target - in;                       // 16.16
	int32_t err_d = err - *last_err;                 // 16.16
	*err_int += (int32_t)ki * (err >> 8);            // 16.16
	*cur_out += (int32_t)kp * (err >> 8) + *err_int; // 16.16
	*cur_out += (int32_t)kd * (err_d >> 8);          // 16.16
	*last_err = err;                                 // 16.16
}

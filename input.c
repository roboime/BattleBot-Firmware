//
// input.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo tem as principais funções que tratam da entrada
// de dados no programa, como os encoders e os receptores
//

#include "default.h"

#include <avr/cpufunc.h>

#define cur0 "r3"
#define cur1 "r4"
#define overflow_count "r5"

// interrupts dos encoders:
register volatile uint8_t cur0_v asm(cur0);
register volatile uint8_t cur1_v asm(cur1);
register volatile uint8_t overflow_count_v asm(overflow_count);
	
// interrupt externo para ler o encoder esquerdo
ISR (INT0_vect)
{
	cur0_v++;
	if (cur0_v == 0) cur1_v++;
}

// overflow to timer0, apenas para contagem de tempo
ISR (TIMER0_OVF_vect)
{
	overflow_count_v++;
}

#define RECV_MID 375
#define RECV_MIN 281
#define RECV_MAX 469
#define RECV_MULT 45
#define RECV_DENOM 16

// TODO: mudar esse parâmetro quando passar p/ o robô real
#define ENC_DIVIDER 3

#define RECV_SAMPLES 31
uint16_t recv_readings[4][RECV_SAMPLES];
uint8_t cur_order[4][RECV_SAMPLES];
uint8_t cur_reading[4];
volatile uint8_t cur_recv_bit = 0, cur_flag = B100;
volatile uint16_t last_times[4][2];
volatile uint8_t updates[4];
static uint8_t last_read = 0;

#define ENC_FRAMES 32
uint16_t enc_frames[ENC_FRAMES];
uint16_t avg_frames = 0;

uint8_t cur_frame = 0;

#define CLEARR(r) asm("eor "r", "r"")

void input_init()
{
	cur_flag = B1;
	for (uint8_t i = 0; i < 4; i++)
	{
		last_times[i][0] = 0;
		last_times[i][1] = 0;

		cur_reading[i] = 0;
		updates[i] = 0;
		
		for (uint8_t j = 0; j < RECV_SAMPLES; j++)
		{
			recv_readings[i][j] = 0;
			cur_order[i][j] = j;
		}
	}
	
	CLEARR(cur0);
	CLEARR(cur1);
	CLEARR(overflow_count);
	
	for (uint8_t i = 0; i < ENC_FRAMES; i++)
		enc_frames[i] = 0;
}

void input_read_enc()
{
	//register unsigned char curl0_v asm(curl0);
	//register unsigned char curl1_v asm(curl1);
	//register unsigned char curr0_v asm(curr0);
	//register unsigned char curr1_v asm(curr1);

	// Dá pra se virar com o interrupt ligado aqui, porque é ULTRA IMPORTANTE pegar todos
	// os interrupts do encoder
	avg_frames -= enc_frames[cur_frame];
	((uint8_t*)&enc_frames[cur_frame])[0] = cur0_v;
	((uint8_t*)&enc_frames[cur_frame])[1] = cur1_v;
	avg_frames += enc_frames[cur_frame];

	CLEARR(cur0);
	CLEARR(cur1);
	
	if (++cur_frame == get_config()->enc_frames) cur_frame = 0;
}

void input_read_recv()
{
	// Aqui não dá pra deixar o interrupt ligado, mas a gente só desliga o do grupo C
	PCICR = 0;
	for (uint8_t i = 0; i < 4; i++)
		if (flags & (RECV_AVAL0 << i))
		{
			recv_readings[i][cur_reading[i]] = last_times[i][1] - last_times[i][0];
			last_times[i][0] = 0;
			last_times[i][1] = 0;
		}
	PCICR = B010;
	
	// Li o que eu precisava, posso reabilitar os interrupts
	// Bubblesort nas amostras
	for (uint8_t i = 0; i < 4; i++)
	{
		if (!(flags & (RECV_AVAL0 << i))) continue;
	
		uint8_t k = 0;
		for (; k < get_config()->recv_samples; k++)
			if (cur_order[i][k] == cur_reading[i]) break;
		
		while (k > 0 && recv_readings[i][cur_order[i][k]] <= recv_readings[i][cur_order[i][k-1]])
		{
			cur_order[i][k] ^= cur_order[i][k-1];
			cur_order[i][k-1] ^= cur_order[i][k];
			cur_order[i][k] ^= cur_order[i][k-1];
			k--;
		}
		
		while (k < get_config()->recv_samples-1 && recv_readings[i][cur_order[i][k]] >= recv_readings[i][cur_order[i][k+1]])
		{
			cur_order[i][k] ^= cur_order[i][k+1];
			cur_order[i][k+1] ^= cur_order[i][k];
			cur_order[i][k] ^= cur_order[i][k+1];
			k++;
		}

		if (++cur_reading[i] == get_config()->recv_samples) cur_reading[i] = 0;
		flags &= ~(RECV_AVAL0 << i);
	}
}

int16_t recv_get_ch(uint8_t ch)
{
	uint16_t recv = recv_readings[ch][cur_order[ch][get_config()->recv_samples/2]];
	//return recv;
	
	if (recv == 0) return 0;

	if (recv > RECV_MAX) recv = RECV_MAX;
	else if (recv < RECV_MIN) recv = RECV_MIN;

	return ((int16_t)recv - (int16_t)RECV_MID) * RECV_MULT / RECV_DENOM;
}

uint16_t enc_value()
{
	return avg_frames / get_config()->enc_frames;
}

// Interrupt do receptor
ISR (PCINT1_vect)
{
	//register unsigned char overflow_count_v asm(overflow_count);
	PCICR = 0;
	sei();

	wdt_reset();
	uint16_t cur_ticks;
	((uint8_t*)&cur_ticks)[0] = TCNT0;
	((uint8_t*)&cur_ticks)[1] = overflow_count_v;
	uint8_t cur_read = (PINC & (cur_flag)) != 0;
	
	if (cur_read && cur_recv_bit == 0)
		flags |= EXECUTE_ENC;
	
	if (cur_read && !last_read)
	{
		last_times[cur_recv_bit][0] = cur_ticks;
	}
	else if (!cur_read && last_read)
	{
		last_times[cur_recv_bit][1] = cur_ticks;
		switch (cur_recv_bit)
		{
			case 0: flags |= RECV_AVAL0; break;
			case 1: flags |= RECV_AVAL1; break;
			case 2: flags |= RECV_AVAL2; break;
			case 3: flags |= RECV_AVAL3; break;
		}
		
		cur_recv_bit++;
		cur_flag <<= 1;
		
		if (cur_recv_bit == 4)
		{
			cur_recv_bit = 0;
			cur_flag = B1;
		}
	}
	
	last_read = cur_read;
	
	cli();
	PCICR = B010;
}

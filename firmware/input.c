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

// interrupts dos encoders:

// interrupt externo para ler o encoder esquerdo
/*ISR (INT0_vect, ISR_NAKED)
{
	asm volatile("inc %0\n\t"
	             "cpse %0, __zero_reg__\n\t"
	             "dec %1\n\t"
	             "inc %1\n\t"
	             "reti"
	             : "=r" (curl0), "=r" (curl1)
	             : "0" (curl0), "1" (curl1));
}

// interrupt externo para ler o encoder direito
ISR (INT1_vect, ISR_NAKED)
{ 
	asm volatile("inc %0\n\t"
	             "cpse %0, __zero_reg__\n\t"
	             "dec %1\n\t"
	             "inc %1\n\t"
	             "reti"
	             : "=r" (curr0), "=r" (curr1)
	             : "0" (curr0), "1" (curr1));
}*/

volatile uint8_t overflow_count = 0;
volatile uint8_t cur_recv_bit = 0, cur_flag = B100;
volatile uint16_t cur_l = 0, cur_r = 0;
volatile uint16_t last_times[3][2];
volatile uint8_t updates[3];
static uint8_t last_read = 0;

ISR (INT0_vect) { cur_l++; }
ISR (INT1_vect) { cur_r++; }

#define RECV_MID 375
#define RECV_MIN 281
#define RECV_MAX 469
#define RECV_MULT 43
#define RECV_DENOM 16

#define RECV_SAMPLES 3
uint16_t recv_readings[3][RECV_SAMPLES];
uint8_t cur_order[3][RECV_SAMPLES];
uint8_t cur_reading[3];

#define ENC_FRAMES 8
uint16_t enc_frames_l[ENC_FRAMES];
uint16_t avg_frames_l = 0;
uint16_t enc_frames_r[ENC_FRAMES];
uint16_t avg_frames_r = 0;

uint8_t cur_frame = 0;

void input_init()
{
	overflow_count = 0;

	cur_flag = B100;
	for (uint8_t i = 0; i < 3; i++)
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
	
	cur_l = 0;
	cur_r = 0;
	
	for (uint8_t i = 0; i < ENC_FRAMES; i++)
	{
		enc_frames_l[i] = 0;
		enc_frames_r[i] = 0;
	}
}

inline void setreg(uint16_t* d, uint8_t r1, uint8_t r2)
{
	uint8_t* p = (uint8_t*)d;

	*p++ = r1;
	*p   = r2;
}

void input_read_enc()
{
	// Dá pra se virar com o interrupt ligado aqui, porque é ULTRA IMPORTANTE pegar todos
	// os interrupts do encoder
	avg_frames_l -= enc_frames_l[cur_frame];
	enc_frames_l[cur_frame] = cur_l;
	avg_frames_l += enc_frames_l[cur_frame];
	
	avg_frames_r -= enc_frames_r[cur_frame];
	enc_frames_r[cur_frame] = cur_r;
	avg_frames_r += enc_frames_r[cur_frame];
	
	cur_l = 0;
	cur_r = 0;
	
	if (++cur_frame == ENC_FRAMES) cur_frame = 0;
}

void input_read_recv()
{
	// Aqui não dá pra deixar o interrupt ligado, mas a gente só desliga o do grupo C
	PCICR &= ~_BV(PCIE1);
	for (uint8_t i = 0; i < 3; i++)
		if (updates[i]) recv_readings[i][cur_reading[i]] = last_times[i][1] - last_times[i][0];
	PCICR |= _BV(PCIE1);
	
	// Li o que eu precisava, posso reabilitar os interrupts
	// Bubblesort nas amostras
	for (uint8_t i = 0; i < 3; i++)
	{
		if (!updates[i]) continue;
	
		uint8_t k = 0;
		for (; k < RECV_SAMPLES; k++)
			if (cur_order[i][k] == cur_reading[i]) break;
		
		while (k > 0 && recv_readings[i][cur_order[i][k]] <= recv_readings[i][cur_order[i][k-1]])
		{
			cur_order[i][k] ^= cur_order[i][k-1];
			cur_order[i][k-1] ^= cur_order[i][k];
			cur_order[i][k] ^= cur_order[i][k-1];
			k--;
		}
		
		while (k < RECV_SAMPLES-1 && recv_readings[i][cur_order[i][k]] >= recv_readings[i][cur_order[i][k+1]])
		{
			cur_order[i][k] ^= cur_order[i][k+1];
			cur_order[i][k+1] ^= cur_order[i][k];
			cur_order[i][k] ^= cur_order[i][k+1];
			k++;
		}

		if (++cur_reading[i] == RECV_SAMPLES) cur_reading[i] = 0;
		updates[i] = 0;
	}
}

int16_t recv_get_ch(uint8_t ch)
{
	uint16_t recv = recv_readings[ch][cur_order[ch][RECV_SAMPLES/2]];
	//return recv;
	
	if (recv == 0) return 0;

	if (recv > RECV_MAX) recv = RECV_MAX;
	else if (recv < RECV_MIN) recv = RECV_MIN;

	return ((int16_t)recv - (int16_t)RECV_MID) * RECV_MULT / RECV_DENOM;
}

uint16_t enc_left()
{
	return avg_frames_l;
}

uint16_t enc_right()
{
	return avg_frames_r;
}

// overflow to timer0, apenas para contagem de tempo
/*ISR (TIMER0_OVF_vect, ISR_NAKED)
{
	asm volatile("inc %0\n\t"
	             "reti"
	             : "=r" (overflow_count)
	             : "0" (overflow_count));
}*/
ISR (TIMER0_OVF_vect) { overflow_count++; }

// função ticks, para a contagem de tempo (1 tick = 64 ciclos)
static inline uint16_t ticks()
{
    uint8_t m = overflow_count;
    uint8_t t = TCNT0;
    return (m << 8) | t;
}

// Interrupt do receptor
ISR (PCINT1_vect)
{
	wdt_reset();
	uint16_t cur_ticks = ticks();
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
		updates[cur_recv_bit] = 1;
		
		cur_recv_bit++;
		cur_flag <<= 1;
		
		if (cur_recv_bit == 3)
		{
			cur_recv_bit = 0;
			cur_flag = B100;
		}
	}
	
	last_read = cur_read;
}

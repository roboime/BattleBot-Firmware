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

#define curl0 "r3"
#define curl1 "r4"
#define curr0 "r5"
#define curr1 "r6"
#define overflow_count "r7"

// interrupts dos encoders:

// interrupt externo para ler o encoder esquerdo
ISR (INT0_vect, ISR_NAKED)
{
	asm volatile("in  __zero_reg__, 0x3F\n\t"
	             "inc "curl0"\n\t"
	             "brne 1f\n\t"
	             "inc "curl1"\n"
	             "1: out 0x3F, __zero_reg__\n\t"
	             "eor __zero_reg__, __zero_reg__\n\t"
	             "reti");
}

// interrupt externo para ler o encoder direito
ISR (INT1_vect, ISR_NAKED)
{ 
	asm volatile("in  __zero_reg__, 0x3F\n\t"
	             "inc "curr0"\n\t"
	             "brne 1f\n\t"
	             "inc "curr1"\n"
	             "1: out 0x3F, __zero_reg__\n\t"
	             "eor __zero_reg__, __zero_reg__\n\t"
	             "reti");
}

// overflow to timer0, apenas para contagem de tempo
ISR (TIMER0_OVF_vect, ISR_NAKED)
{
	asm volatile("in  __zero_reg__, 0x3F\n\t"
	             "inc "overflow_count"\n\t"
	             "out 0x3F, __zero_reg__\n\t"
	             "eor __zero_reg__, __zero_reg__\n\t"
	             "reti");
}

//volatile uint8_t overflow_count = 0;
volatile uint8_t cur_recv_bit = 0, cur_flag = B100;
//volatile uint16_t cur_l = 0, cur_r = 0;
volatile uint16_t last_times[3][2];
volatile uint8_t updates[3];
static uint8_t last_read = 0;

#define RECV_MID 375
#define RECV_MIN 281
#define RECV_MAX 469
#define RECV_MULT 43
#define RECV_DENOM 16

#define RECV_SAMPLES 31
uint16_t recv_readings[3][RECV_SAMPLES];
uint8_t cur_order[3][RECV_SAMPLES];
uint8_t cur_reading[3];

#define ENC_FRAMES 32
uint16_t enc_frames_l[ENC_FRAMES];
uint16_t avg_frames_l = 0;
uint16_t enc_frames_r[ENC_FRAMES];
uint16_t avg_frames_r = 0;

uint8_t cur_frame = 0;

#define CLEARR(r) asm("eor "r", "r"")

void input_init()
{
	cur_flag = B100;
	for (uint8_t i = 0; i < 3; i++)
	{
		last_times[i][0] = 0;
		last_times[i][1] = 0;

		cur_reading[i] = 0;
		updates[i] = 0;
		
		for (uint8_t j = 0; j < get_config()->recv_samples; j++)
		{
			recv_readings[i][j] = 0;
			cur_order[i][j] = j;
		}
	}
	
	CLEARR(curl0);
	CLEARR(curl1);
	CLEARR(curr0);
	CLEARR(curr1);
	CLEARR(overflow_count);
	
	for (uint8_t i = 0; i < get_config()->enc_frames; i++)
	{
		enc_frames_l[i] = 0;
		enc_frames_r[i] = 0;
	}
}

void input_read_enc()
{
	register unsigned char curl0_v asm(curl0);
	register unsigned char curl1_v asm(curl1);
	register unsigned char curr0_v asm(curr0);
	register unsigned char curr1_v asm(curr1);

	// Dá pra se virar com o interrupt ligado aqui, porque é ULTRA IMPORTANTE pegar todos
	// os interrupts do encoder
	avg_frames_l -= enc_frames_l[cur_frame];
	((uint8_t*)&enc_frames_l[cur_frame])[0] = curl0_v;
	((uint8_t*)&enc_frames_l[cur_frame])[1] = curl1_v;
	avg_frames_l += enc_frames_l[cur_frame];
	
	avg_frames_r -= enc_frames_r[cur_frame];
	((uint8_t*)&enc_frames_r[cur_frame])[0] = curr0_v;
	((uint8_t*)&enc_frames_r[cur_frame])[1] = curr1_v;
	avg_frames_r += enc_frames_r[cur_frame];
	
	CLEARR(curl0);
	CLEARR(curr0);
	CLEARR(curl1);
	CLEARR(curr1);
	
	if (++cur_frame == get_config()->enc_frames) cur_frame = 0;
}

void input_read_recv()
{
	// Aqui não dá pra deixar o interrupt ligado, mas a gente só desliga o do grupo C
	PCICR &= ~_BV(PCIE1);
	for (uint8_t i = 0; i < 3; i++)
		if (flags & (RECV_AVAL0 << i))
		{
			recv_readings[i][cur_reading[i]] = last_times[i][1] - last_times[i][0];
			last_times[i][0] = 0;
			last_times[i][1] = 0;
		}
	PCICR |= _BV(PCIE1);
	
	// Li o que eu precisava, posso reabilitar os interrupts
	// Bubblesort nas amostras
	for (uint8_t i = 0; i < 3; i++)
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

uint16_t enc_left()
{
	return avg_frames_l;
}

uint16_t enc_right()
{
	return avg_frames_r;
}

// Interrupt do receptor
ISR (PCINT1_vect)
{
	register unsigned char overflow_count_v asm(overflow_count);

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
		}
		
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

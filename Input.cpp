//////////////////////////////////////////////////
// Input.cpp                                    //
//////////////////////////////////////////////////
// Módulo responsável por gerenciar a entrada   //
// de sinal do programa.                        //
//////////////////////////////////////////////////

#include <Arduino.h>
#include <Filters.h>
#include "Header.hpp"

FilterOnePole lowpassFilter(LOWPASS, 1.8);

// Precisamos definir um interrupt para o grupo C

// Todos os pinos
#define C_INPUT_MASK B11101

// Encoder
#define C_INTERRUPT_MASK B00001

// Receptor
#define C_RECEPTOR_MASK0 B00100
#define C_RECEPTOR_MASK1 B01000
#define C_RECEPTOR_MASK2 B10000

// Pinos da dip-switch de configuração da placa
#define IN_DIPSWITCH_0 4
#define IN_DIPSWITCH_1 8

// Tratamento do preocessamento de sinal do encoder
// O sinal precisa ser tratado com o filtro de médias móveis
// para que não saia extremamente ruidoso.
#define FRAME_TIME 20000
#define FRAME_COUNT 5

// mapa do tempo do receptor para o intervalo [-100,100]
const struct { unsigned int min, max; }
receptorIntervals[3] = { { 139, 237 }, { 144, 236 }, { 120, 256 } };

// se o receptor ficar mais de esse tempo em us sem mandar
// uma onda de PWM, indicar que o controle foi perdido
#define SIGNAL_LOSS_LIMIT 3000

// Variáveis para o tratamento de sinal
unsigned long frameCount[FRAME_COUNT];
unsigned long speedMark;
unsigned char currentFrame;
volatile unsigned long curFrameCount;

// Variáveis para a leitura do sinal dos receptores
unsigned int localReceptorReadings[3];
unsigned int lastReadings[3];
volatile unsigned int receptorReadings[3];
bool copyDone;
long signalCounter;

// ISR do grupo de pinos C
// Leitura do pino de encoder
ISR (PCINT1_vect) { curFrameCount++; }

// ISR do timer 2 para o polling do receptor
ISR(TIMER2_COMPA_vect)
{
    char curC = PINC;

    if (curC & C_RECEPTOR_MASK0) localReceptorReadings[0]++;
    else if (localReceptorReadings[0] != 0)
    {
        receptorReadings[0] = localReceptorReadings[0];
        localReceptorReadings[0] = 0;
    }
    
    if (curC & C_RECEPTOR_MASK1) localReceptorReadings[1]++;
    else if (localReceptorReadings[1] != 0)
    {
        receptorReadings[1] = localReceptorReadings[1];
        localReceptorReadings[1] = 0;
    }
    
    if (curC & C_RECEPTOR_MASK2) { localReceptorReadings[2]++; signalCounter = 0; }
    else if (localReceptorReadings[2] != 0)
    {
        receptorReadings[2] = localReceptorReadings[2];
        localReceptorReadings[2] = 0;
    }
    else signalCounter++;
}

// Inicialização do módulo
void inSetup()
{
    noInterrupts();
    
    // Inicializar os interrupts do grupo C
    DDRC &= ~C_INPUT_MASK; // define como pinos de entrada
    PORTB |= C_INPUT_MASK; // habilita o resistor pull-down
    PCMSK1 |= C_INTERRUPT_MASK; // habilita o interrupt para o encoder

    // Define o timer
    TCCR2A = 1 << WGM21; // modo CTC
    TCCR2B = 1 << CS20; // sem prescaler
    OCR2A = 128; // 64 ciclos por pulso

    // interrupts
    PCICR |= 1 << PCIE1;
    TIMSK2 |= 1 << OCIE2A;

    // inicialização das variáveis dos receptores
    for (char i = 0; i < 3; i++)
    {
        localReceptorReadings[i] = 0;
        lastReadings[i] = 0;
        receptorReadings[i] = 0;
    }

    // inicialização das variáveis dos encoders
    for (char i = 0; i < FRAME_COUNT; i++)
        frameCount[i] = 0;
    speedMark = 0;
    curFrameCount = 0;
    currentFrame = 0;
    copyDone = false;

    signalCounter = 0;

    interrupts();

    // dip-switch
    pinMode(IN_DIPSWITCH_0, INPUT);
    pinMode(IN_DIPSWITCH_1, INPUT);
}

// Loop principal do módulo de input
void inLoop()
{
    static unsigned long lastTime = micros();
    unsigned long curTime = micros();

    // percepção discreta de quadros
    if (curTime - lastTime > FRAME_TIME)
    {
        // filtro passa baixa
        speedMark = lowpassFilter.input(curFrameCount);

        if (++currentFrame >= FRAME_COUNT) currentFrame = 0;
        curFrameCount = 0;

        while (curTime - lastTime > FRAME_TIME)
            lastTime += FRAME_TIME;
    }
}

// funções voltadas para "pegar" o input
int inGetReceptorReadings(int channel)
{
    if (signalCounter > SIGNAL_LOSS_LIMIT) return 0;
  
    int val = map(receptorReadings[channel], receptorIntervals[channel].min, receptorIntervals[channel].max, -100, 100);
    if (val < -112 || val > 112) return lastReadings[channel];
    if (val < -100) val = -100; else if (val > 100) val = 100;
    return lastReadings[channel] = val;

    //return receptorReadings[channel];
}

unsigned long inGetSpeed()
{
    return speedMark;
}

bool dipSwitch(int port)
{
    return digitalRead(port ? IN_DIPSWITCH_1 : IN_DIPSWITCH_0) == HIGH;
}




//////////////////////////////////////////////////
// Input.cpp                                    //
//////////////////////////////////////////////////
// Módulo responsável por gerenciar a entrada   //
// de sinal do programa.                        //
//////////////////////////////////////////////////

#include <Arduino.h>
#include "Header.hpp"

// Precisamos definir dois interrupts: um para o grupo de
// pinos D e um para o grupo C, além dos dois interrupts
// externos. Essas constantes facilitam a rápida leitura
// dos pinos

// Pinos do receptor
#define C_INPUT_MASK B11111
#define C_RECEPTOR_MASK0 B00100
#define C_RECEPTOR_MASK1 B01000
#define C_RECEPTOR_MASK2 B10000
#define C_ENCODER_MASK0 B00001
#define C_ENCODER_MASK1 B00010

// Pinos da dip-switch de configuração da placa
#define IN_DIPSWITCH_0 4
#define IN_DIPSWITCH_1 8

// Tratamento do preocessamento de sinal do encoder
// O sinal precisa ser tratado com o filtro de médias móveis
// para que não saia extremamente ruidoso.
#define FRAME_TIME 20000
#define FRAME_COUNT 5

// mapa do tempo do receptor para o intervalo [-100,100]
#define RECEPTOR_MIN 1120
#define RECEPTOR_MAX 1916

// se o receptor ficar mais de esse tempo em us sem mandar
// uma onda de PWM, indicar que o controle foi perdido
#define SIGNAL_LOSS_LIMIT 27000

// Variáveis para o tratamento de sinal
unsigned long frameCount[FRAME_COUNT];
unsigned long speedMark;
unsigned char currentFrame;
volatile unsigned long curFrameCount;
volatile unsigned char dirBit;

// Variáveis para a leitura do sinal dos receptores
unsigned char lastC;
volatile unsigned long lastTimes[3][2];
unsigned long receptorReadings[3];
bool signalLost;

// ISR do grupo de pinos C
// Leitura dos dois últimos pinos do encoder
ISR (PCINT1_vect)
{
    unsigned long time = micros();

    unsigned char curC = PINC & C_INPUT_MASK;
    unsigned char diff = curC ^ lastC;

    if (diff & C_RECEPTOR_MASK0) lastTimes[0][!(curC & C_RECEPTOR_MASK0)] = time;
    if (diff & C_RECEPTOR_MASK1) lastTimes[1][!(curC & C_RECEPTOR_MASK1)] = time;
    if (diff & C_RECEPTOR_MASK2) lastTimes[2][!(curC & C_RECEPTOR_MASK2)] = time;

    if (diff & C_ENCODER_MASK0) curFrameCount++;

    lastC = curC;
}

// Inicialização do módulo
void inSetup()
{
    // Inicializar os interrupts do grupo C
    DDRC &= ~C_INPUT_MASK;
    PCMSK1 |= C_INPUT_MASK;
    lastC = PINC & C_INPUT_MASK;

    // interrupt geral
    PCICR |= (1 << PCIE1);

    // inicialização das variáveis dos receptores
    for (char i = 0; i < 3; i++)
    {
        lastTimes[i][0] = lastTimes[i][1] = 0;
        receptorReadings[i] = 0;
    }

    // inicialização das variáveis dos encoders
    for (char i = 0; i < FRAME_COUNT; i++)
        frameCount[i] = 0;
    speedMark = 0;
    curFrameCount = 0;
    currentFrame = 0;
    dirBit = 0;

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
        // média móvel
        speedMark -= frameCount[currentFrame];
        frameCount[currentFrame] = curFrameCount;
        speedMark += frameCount[currentFrame];
        //speedMark = curFrameCount;

        if (++currentFrame >= FRAME_COUNT) currentFrame = 0;
        curFrameCount = 0;

        while (curTime - lastTime > FRAME_TIME)
            lastTime += FRAME_TIME;
    }

    // leitura do receptor
    unsigned char lowBits = ~((lastC >> 2) & B111);

    signalLost = false;

    for (char i = 0; i < 3; i++)
    {
        if ((lowBits & (i << i)) && lastTimes[i][1] != 0)
        {
            receptorReadings[i] = lastTimes[i][1] - lastTimes[i][0];
            lastTimes[i][1] = 0;
        }

        if (curTime - lastTimes[i][0] > SIGNAL_LOSS_LIMIT) signalLost = true;
    }
}

// funções voltadas para "pegar" o input
int inGetReceptorReadings(int channel)
{
    return map(receptorReadings[channel], RECEPTOR_MIN, RECEPTOR_MAX, -100, 100);
}

unsigned long inGetSpeed()
{
    return speedMark;
}

bool inSignalLost()
{
    return signalLost;
}

bool dipSwitch(int port)
{
    return digitalRead(port ? IN_DIPSWITCH_1 : IN_DIPSWITCH_0) == HIGH;
}



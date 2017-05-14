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

// Pinos PC2 e PC1 (A2 e A1 do Arduino)
#define C_RECEPTOR_MASK B110
#define C_MASK0 B010
#define C_MASK1 B100

// Pinos da dip-switch de configuração da placa
#define IN_DIPSWITCH_0 8
#define IN_DIPSWITCH_1 10

// Tratamento do preocessamento de sinal do encoder
// O sinal precisa ser tratado com o filtro de médias móveis
// para que não saia extremamente ruidoso.
#define FRAME_TIME 20000
#define FRAME_COUNT 10

// mapa do tempo do receptor para o intervalo [-100,100]
const struct { int min, max; }
receptorInterval[2] = { { 1116, 1912 }, { 1140, 1940 } };

// se o receptor ficar mais de esse tempo em us sem mandar
// uma onda de PWM, indicar que o controle foi perdido
#define SIGNAL_LOSS_LIMIT 27000

// Variáveis para o tratamento de sinal
unsigned char frameCount[FRAME_COUNT];
unsigned int speedMark;
unsigned char currentFrame;
volatile unsigned curFrameCount;
volatile unsigned char dirBit;

// Variáveis para a leitura do sinal dos receptores
unsigned char lastC;
volatile unsigned long lastTimes[2][2];
unsigned long receptorReadings[2];
bool signalLost;

// ISR do grupo de pinos C
// Leitura dos dois últimos pinos do encoder
ISR (PCINT1_vect)
{
    unsigned long time = micros();

    unsigned char curC = PINC & C_RECEPTOR_MASK;
    unsigned char diff = curC ^ lastC;

    if (diff & C_MASK0) lastTimes[0][!(curC & C_MASK0)] = time;
    if (diff & C_MASK1) lastTimes[1][!(curC & C_MASK1)] = time;

    lastC = curC;
}

// Interrupts externos associados aos pinos do encoder
ISR (INT0_vect)
{
    curFrameCount++;
}

// Inicialização do módulo
void inSetup()
{
    // grupo C
    DDRC &= ~C_RECEPTOR_MASK; // pinos de entrada
    PCMSK1 |= C_RECEPTOR_MASK;
    lastC = PINC & C_RECEPTOR_MASK;

    // interrupt geral
    PCICR |= (1 << PCIE1);

    // interrupts externos
    DDRD &= ~B0100;
    EICRA = B0001;
    EIMSK = 1;

    // inicialização das variáveis dos receptores
    for (int i = 0; i < 2; i++)
    {
        lastTimes[i][0] = lastTimes[i][1] = 0;
        receptorReadings[i] = 0;
    }

    // inicialização das variáveis dos encoders
    for (int i = 0; i < FRAME_COUNT; i++)
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
    unsigned char lowBits = ~(lastC & C_RECEPTOR_MASK) >> 1;

    signalLost = false;

    for (char i = 0; i < 2; i++)
    {
        if ((lowBits & (1 << i)) && lastTimes[i][1] != 0)
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
    int val = map(receptorReadings[channel], receptorInterval[channel].min, receptorInterval[channel].max, -100, 100);
    if (val < -100) val = -100; else if (val > 100) val = 100;
    return val;
}

int inGetSpeed()
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



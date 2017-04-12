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

// Pinos PD2 e PD3 (10 e 11 do Arduino)
#define D_RECEPTOR_MASK B1100
#define D_MASK0 B0100
#define D_MASK1 B1000

// Pinos PC2 e PC1 (A2 e A1 do Arduino)
#define C_RECEPTOR_MASK B110
#define C_MASK2 B100
#define C_MASK3 B010

// Pinos do encoder, para os interrupts externos
#define IN_ENCODER_0 2
#define IN_ENCODER_1 3

// Pinos da dip-switch de configuração da placa
#define IN_DIPSWITCH_0 8
#define IN_DIPSWITCH_1 10

// Tratamento do preocessamento de sinal do encoder
// O sinal precisa ser tratado com o filtro de médias móveis
// para que não saia extremamente ruidoso.
#define FRAME_TIME 20000
#define FRAME_COUNT 10

// mapa do tempo do receptor para o intervalo [-100,100]
#define RECEPTOR_MIN 1120
#define RECEPTOR_MAX 1916

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
unsigned char lastD, lastC;
volatile unsigned long lastTimes[4][2];
unsigned long receptorReadings[4];
bool signalLost;

// ISR do grupo de pinos D
// Leitura dos dois primeiros pinos do encoder
ISR (PCINT2_vect)
{
    unsigned long time = micros();

    unsigned char curD = PIND & D_RECEPTOR_MASK;
    unsigned char diff = curD ^ lastD;

    if (diff & D_MASK0) lastTimes[0][!(curD & D_MASK0)] = time;
    if (diff & D_MASK1) lastTimes[1][!(curD & D_MASK1)] = time;

    lastD = curD;
}

// ISR do grupo de pinos C
// Leitura dos dois últimos pinos do encoder
ISR (PCINT1_vect)
{
    unsigned long time = micros();

    unsigned char curC = PINC & C_RECEPTOR_MASK;
    unsigned char diff = curC ^ lastC;

    if (diff & C_MASK2) lastTimes[2][!(curC & C_MASK2)] = time;
    if (diff & C_MASK3) lastTimes[3][!(curC & C_MASK3)] = time;

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
    // Inicializar os interrupts do grupo D
    //DDRD &= ~D_RECEPTOR_MASK;
    //PCMSK2 |= D_RECEPTOR_MASK;
    //lastD = PIND & D_RECEPTOR_MASK;

    // grupo C
    DDRC &= ~C_RECEPTOR_MASK;
    PCMSK1 |= C_RECEPTOR_MASK;
    lastC = PINC & C_RECEPTOR_MASK;

    // interrupt geral
    PCICR |= (1 << PCIE1)/* | (1 << PCIE2)*/;

    // interrupts externos
    DDRD &= ~B0100;
    EICRA = B0001;
    EIMSK = 1;

    // inicialização das variáveis dos receptores
    for (char i = 0; i < 4; i++)
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
        //speedMark -= frameCount[currentFrame];
        //frameCount[currentFrame] = curFrameCount;
        //speedMark += frameCount[currentFrame];
        speedMark = curFrameCount;

        if (++currentFrame >= FRAME_COUNT) currentFrame = 0;
        curFrameCount = 0;

        while (curTime - lastTime > FRAME_TIME)
            lastTime += FRAME_TIME;
    }

    // leitura do receptor
    unsigned char lowBits = (!(lastD & D_MASK0) << 0) ||
                            (!(lastD & D_MASK1) << 1) ||
                            (!(lastC & C_MASK2) << 2) ||
                            (!(lastC & C_MASK3) << 3);

    signalLost = false;

    for (char i = 0; i < 4; i++)
    {
        if (((lowBits >> i) & 1) && lastTimes[i][1] != 0)
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



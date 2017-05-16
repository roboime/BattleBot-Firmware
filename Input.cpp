//////////////////////////////////////////////////
// Input.cpp                                    //
//////////////////////////////////////////////////
// Módulo responsável por gerenciar a entrada   //
// de sinal do programa.                        //
//////////////////////////////////////////////////

#include <Arduino.h>
#include <Filters.h>
#include "Header.hpp"

typedef unsigned char u8;

// Interrupt PCINT2: PD2 e PD3 para o encoder do motor esquerdo
#define D_INPUT_MASK B1100

// Interrupt PCINT0: PB3 e PB4 para o encoder do motor direito
#define B_INPUT_MASK B11000

// Interrupt PCINT1: PC3, PC4 e PC5 para os sinais do controle
#define C_INPUT_MASK B111000
#define C_MASK0 B001000
#define C_MASK1 B010000
#define C_MASK2 B100000

// Pinos da dip-switch de configuração da placa
char dipSwitches[] = { 4, 7, 8 };

// Tratamento do preocessamento de sinal do encoder
// O sinal precisa ser tratado com o filtro de médias móveis
// para que não saia extremamente ruidoso.
#define FRAME_TIME 20000
#define FRAME_COUNT 5

// mapa do tempo do receptor para o intervalo [-100,100]
const struct { unsigned int min, max; }
receptorInterval[3] = { { 1120, 1920 }, { 1140, 1932 }, { 1020, 2020 } };

// se o receptor ficar mais de esse tempo em us
// sem mandar sinal, consideramos o sinal perdido
#define SIGNAL_LOSS_LIMIT 25600

// Variáveis para o tratamento de sinal no encoder esquerdo
u8 lastD;
int frameCountL[FRAME_COUNT];
int speedMarkL;
volatile int curFrameCountL;

// Variáveis para o tratamento de sinal no encoder direito
u8 lastB;
int frameCountR[FRAME_COUNT];
int speedMarkR;
volatile int curFrameCountR;

// Variáveis comuns
unsigned char currentFrame;

// Variáveis para a leitura do sinal dos receptores
u8 lastC;
volatile unsigned long lastTimes[3][2];
unsigned long receptorReadings[3];
u8 ports[3];
bool signalLost;

// Tabela de lookup para os padrões de gray code
const u8 grayCodeTable[16] =
{
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0,
};

// ISR do grupo de pinos D
// Leitura do encoder esquerdo
ISR (PCINT2_vect)
{
    // Feio mexer com código assim, mas já que precisa... (não é pra tirar esses (u8) :/)
    u8 curD = (u8)((u8)PIND & (u8)D_INPUT_MASK);
    //curFrameCountL += grayCodeTable[lastD & (curD >> 2)];
    curFrameCountL += grayCodeTable[(u8)((u8)lastD & (u8)((u8)curD >> (u8)2))];
    lastD = curD;
}

// ISR do grupo de pinos C
// Leitura do encoder esquerdo
ISR (PCINT0_vect)
{
    u8 curB = (u8)((u8)PINB & (u8)B_INPUT_MASK);
    //curFrameCountR += grayCodeTable[(lastB >> 1) & (curB >> 3)];
    curFrameCountR += grayCodeTable[(u8)((u8)((u8)lastB >> (u8)1) & (u8)((u8)curB >> (u8)3))];
    lastB = curB;
}

// ISR do grupo de pinos C
// Leitura dos pinos do receptor
ISR (PCINT1_vect)
{
    unsigned long time = micros();

    u8 curC = (u8)((u8)PINC & (u8)C_INPUT_MASK);
    u8 diff = (u8)((u8)curC ^ (u8)lastC);

    if ((u8)diff & (u8)C_MASK0) lastTimes[0][ports[0] = !ports[0]] = time;
    if ((u8)diff & (u8)C_MASK1) lastTimes[1][ports[1] = !ports[1]] = time;
    if ((u8)diff & (u8)C_MASK2) lastTimes[2][ports[2] = !ports[2]] = time;

    lastC = curC;
}

// Inicialização do módulo
void inSetup()
{
    noInterrupts();
    // grupo D, otimizações para reduzir o tamanho do código compilado
    DDRD = ~D_INPUT_MASK;
    PCMSK2 = D_INPUT_MASK;
    lastD = (u8)((u8)PIND & (u8)D_INPUT_MASK);

    // grupo B
    DDRB = ~B_INPUT_MASK;
    PCMSK0 = B_INPUT_MASK;
    lastB = (u8)((u8)PINB & (u8)B_INPUT_MASK);
 
    // grupo C
    DDRC = ~C_INPUT_MASK;
    PCMSK1 = C_INPUT_MASK;
    lastC = (u8)((u8)PINC & (u8)C_INPUT_MASK);

    // interrupt geral
    PCICR = B111;

    // inicialização das variáveis dos receptores
    for (int i = 0; i < 3; i++)
    {
        localReceptorReadings[i] = 0;
        lastReadings[i] = 0;
        receptorReadings[i] = 0;
    }

    ports[0] = !((u8)PINC & (u8)C_MASK0);
    ports[1] = !((u8)PINC & (u8)C_MASK1);
    ports[2] = !((u8)PINC & (u8)C_MASK2);

    // inicialização das variáveis dos encoders
    for (int i = 0; i < FRAME_COUNT; i++)
        frameCountL[i] = frameCountR[i] = 0;
    speedMarkL = speedMarkR = 0;
    curFrameCountL = curFrameCountR = 0;
    currentFrame = 0;

    // dip-switch
    for (int i = 0; i < 3; i++)
        pinMode(dipSwitches[i], INPUT_PULLUP);

    interrupts();
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
        speedMarkL -= frameCountL[currentFrame];
        frameCountL[currentFrame] = curFrameCountL;
        speedMarkL += frameCountL[currentFrame];

        speedMarkR -= frameCountR[currentFrame];
        frameCountR[currentFrame] = curFrameCountR;
        speedMarkR += frameCountR[currentFrame];

        if (++currentFrame >= FRAME_COUNT) currentFrame = 0;
        curFrameCountL = curFrameCountR = 0;

        while (curTime - lastTime > FRAME_TIME)
            lastTime += FRAME_TIME;
    }

    // leitura do receptor, otimização
    //u8 lowBits = ~(lastC & C_INPUT_MASK) >> 3;
    u8 lowBits = (u8)((u8)~((u8)lastC & (u8)C_INPUT_MASK) >> (u8)3);

    signalLost = false;

    for (char i = 0; i < 3; i++)
    {
        if (((u8)lowBits & (u8)(1 << i)) && lastTimes[i][1] != 0)
        {
            receptorReadings[i] = lastTimes[i][1] - lastTimes[i][0];
            lastTimes[i][1] = 0;
        }

        if (curTime - lastTimes[i][0] > SIGNAL_LOSS_LIMIT) signalLost = true;
    }
}

// funções voltadas para "pegar" o input
int inGetReceptorReadings(char channel)
{
    if (signalLost) return 0;
  
    int val = map(receptorReadings[channel], receptorInterval[channel].min, receptorInterval[channel].max, -100, 100);
    if (val < -100) val = -100; else if (val > 100) val = 100;
    return val;
}

int inGetSpeedLeft()
{
    return speedMarkL;
}

int inGetSpeedRight()
{
    return speedMarkR;
}

bool dipSwitch(int port)
{
    return digitalRead(dipSwitches[port]) == HIGH;
}




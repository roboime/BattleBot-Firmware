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
#define N_SAMPLES 7
volatile unsigned long lastTimes[3][2];
char curMask, curBit, lastRead;
unsigned long receptorReadings[3][N_SAMPLES];

// filtro de mediana
char order[3][N_SAMPLES];
char curMedianSample[3];

// mapa do tempo do receptor para o intervalo [-100,100]
const struct { long min, max; }
receptorInterval[3] = { { 1124, 1880 }, { 1144, 1904 }, { 1060, 1980 } };

// se o receptor ficar mais de esse tempo em us
// sem mandar sinal, consideramos o sinal perdido
#define SIGNAL_LOSS_LIMIT 40000
#define SIGNAL_LOSS_FADE 360000
unsigned long timeSinceSignalLost;

// Filtro de atenuação dos receptores, em u/256
#define K 256

// Tabela de lookup para os padrões de gray code
signed char grayCodeTable[16] =
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
    u8 curD = PIND & D_INPUT_MASK;
    //curFrameCountL += grayCodeTable[lastD & (curD >> 2)];
    curFrameCountL += grayCodeTable[(u8)((u8)lastD & (u8)((u8)curD >> (u8)2))];
    lastD = curD;
}

// ISR do grupo de pinos C
// Leitura do encoder esquerdo
ISR (PCINT0_vect)
{
    u8 curB = PINB & B_INPUT_MASK;
    //curFrameCountR += grayCodeTable[(lastB >> 1) & (curB >> 3)];
    curFrameCountR += grayCodeTable[(u8)((u8)((u8)lastB >> (u8)1) & (u8)((u8)curB >> (u8)3))];
    lastB = curB;
}

// ISR do grupo de pinos C
// Leitura dos pinos do receptor
ISR (PCINT1_vect)
{
    bool curRead = ((u8)PINC & (u8)curMask) != 0;

    if (curRead) lastTimes[curBit][0] = micros();
    else if (lastRead)
    {
        lastTimes[curBit][1] = micros();
        curMask <<= (u8)1;
        curBit++;

        if (curBit == 3)
        {
            curMask = C_MASK0;
            curBit = 0;
        }
    }

    lastRead = curRead;
}

void resetReceptorStatus()
{
    for (int i = 0; i < 3; i++)
    {
        lastTimes[i][0] = lastTimes[i][1] = 0;
        curMedianSample[i] = 0;

        for (int j = 0; j < N_SAMPLES; j++)
        {
            receptorReadings[i][j] = 0;
            order[i][j] = j;
        }
    }

    curBit = 0;
    curMask = C_MASK0;
}

// Inicialização do módulo
void inSetup()
{
    noInterrupts();
    // grupo D, otimizações para reduzir o tamanho do código compilado
    DDRD &= ~D_INPUT_MASK;
    PCMSK2 |= D_INPUT_MASK;
    lastD = (u8)((u8)PIND & (u8)D_INPUT_MASK);

    // grupo B
    DDRB = ~B_INPUT_MASK;
    PCMSK0 |= B_INPUT_MASK;
    lastB = (u8)((u8)PINB & (u8)B_INPUT_MASK);
 
    // grupo C
    DDRC &= ~C_INPUT_MASK;
    PORTC |= C_INPUT_MASK;
    PCMSK1 |= C_INPUT_MASK;

    lastRead = false;

    // interrupt geral
    PCICR = B111;

    // inicialização das variáveis dos encoders
    for (int i = 0; i < FRAME_COUNT; i++)
        frameCountL[i] = frameCountR[i] = 0;
    speedMarkL = speedMarkR = 0;
    curFrameCountL = curFrameCountR = 0;
    currentFrame = 0;

    resetReceptorStatus();
    timeSinceSignalLost = 0;

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
        //speedMarkL -= frameCountL[currentFrame];
        //frameCountL[currentFrame] = curFrameCountL;
        //speedMarkL += frameCountL[currentFrame];

        //speedMarkR -= frameCountR[currentFrame];
        //frameCountR[currentFrame] = curFrameCountR;
        //speedMarkR += frameCountR[currentFrame];

        speedMarkL = curFrameCountL;
        speedMarkR = curFrameCountR;

        if (++currentFrame >= FRAME_COUNT) currentFrame = 0;
        curFrameCountL = curFrameCountR = 0;

        while (curTime - lastTime > FRAME_TIME)
            lastTime += FRAME_TIME;
    }
    
    // leitura do receptor

    bool updates[3] = { false, false, false };
    noInterrupts();
    long globalLastTime = 0;
    for (int i = 0; i < 3; i++)
    {
        if (lastTimes[i][1] != 0)
        {
            receptorReadings[i][curMedianSample[i]] = lastTimes[i][1] - lastTimes[i][0];
            updates[i] = true;
            lastTimes[i][1] = 0;
        }

        if (globalLastTime < lastTimes[i][0])
            globalLastTime = lastTimes[i][0];
    }
    interrupts();

    for (int i = 0; i < 3; i++)
    {
        if (updates[i])
        {
            // bubble the result
            int k;
            for (k = 0; k < N_SAMPLES; k++) if (order[i][k] == curMedianSample[i]) break;
            if (k == N_SAMPLES) continue;
            
            while (k > 0 && receptorReadings[i][order[i][k]] <= receptorReadings[i][order[i][k-1]])
            {
                char tmp = order[i][k];
                order[i][k] = order[i][k-1];
                order[i][k-1] = tmp;
                k--;
            }

            while (k < N_SAMPLES-1 && receptorReadings[i][order[i][k]] >= receptorReadings[i][order[i][k+1]])
            {
                char tmp = order[i][k];
                order[i][k] = order[i][k+1];
                order[i][k+1] = tmp;
                k++;
            }
            
            if (++curMedianSample[i] == N_SAMPLES) curMedianSample[i] = 0;
        }
    }
    
    timeSinceSignalLost = 0;
    if (curTime > globalLastTime + SIGNAL_LOSS_LIMIT)
        timeSinceSignalLost = curTime - globalLastTime - SIGNAL_LOSS_LIMIT;
    if (timeSinceSignalLost > SIGNAL_LOSS_FADE)
    {
        timeSinceSignalLost = SIGNAL_LOSS_FADE;
        resetReceptorStatus();
    }
}

// funções voltadas para "pegar" o input
int inGetReceptorReadings(char channel)
{
    unsigned long ch = receptorReadings[channel][order[channel][N_SAMPLES/2]];
  
    if (ch == 0) return 0;
  
    int val = map((long)ch, receptorInterval[channel].min, receptorInterval[channel].max, -250, 250);
    if (val < -250) val = -250; else if (val > 250) val = 250;

    if (timeSinceSignalLost > 0)
    {
        int decay = map((long)timeSinceSignalLost, 0, SIGNAL_LOSS_FADE, 0, 250);
        if (val < 0) val = min(val + decay, 0);
        else val = max(val - decay, 0);
    }
    
    return val;

    //return receptorReadings[channel];
}

int inGetSpeedLeft()
{
    return speedMarkL;
}

int inGetSpeedRight()
{
    return speedMarkR;
}

bool isSignalLost()
{
    return false;
}

bool dipSwitch(int port)
{
    return digitalRead(dipSwitches[port]) == HIGH;
}




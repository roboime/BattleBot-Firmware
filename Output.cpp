///////////////////////////////////////////////////
// Output.cpp                                    //
///////////////////////////////////////////////////
// Módulo responsável por controlar o sinal      //
// emitido pelo código principal para os motores //
// e o LED.                                      //
///////////////////////////////////////////////////

#include <Arduino.h>
#include "Header.hpp"

// Pinos de saída para o circuito da ponte H e
// do LED
#define M_IN1 5
#define M_IN2 6
#define M_SD 7
#define LED 9

// Potência "máxima" do motor, se ficar muito
// perto de 255, o motor para
#define MOTOR_MAX_POWER 220

void outSetup()
{
    pinMode(M_IN1, OUTPUT);
    pinMode(M_IN2, OUTPUT);
    pinMode(M_SD, OUTPUT);
    pinMode(LED, OUTPUT);
}

// LED controlado por PWM
void outSetLed(char val)
{
    analogWrite(LED, val);
}

void outSetMotorEnabled(bool val)
{
    digitalWrite(M_SD, val ? HIGH : LOW);
}

// O esquema de fase-magnitude para o controle do
// motor é definido assim: para o fazer o motor
// girar em um sentido, aplica-se PWM em um pino,
// enquanto o outro é mantido LOW. Para o motor
// girar no outro sentido, invertem-se os pinos
// do PWM e do LOW.
// Essa função aceita passos no intervalo
// [-100,+100]

void outSetMotorPower(char step)
{
    if (step == 0)
    {
        digitalWrite(M_IN1, LOW);
        digitalWrite(M_IN2, LOW);
    }
    else if (step > 0)
    {
        analogWrite(M_IN1, map(step, 0, 100, 0, 220));
        digitalWrite(M_IN2, LOW);
    }
    else if (step < 0) // redundante, mas é pra ficar claro
    {
        digitalWrite(M_IN1, LOW);
        analogWrite(M_IN2, map(-step, 0, 100, 0, 220));
    }
    }
}



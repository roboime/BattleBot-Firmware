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
const struct { char in1, in2; }
motorPins[] = { { 5, 6 }, { 9, 10 } };

const char leds[] = { A0, A1 };

// Potência "máxima" do motor, se ficar muito
// perto de 255, o motor para
#define MOTOR_MAX_POWER 250

void outSetup()
{
    pinMode(motorPins[0].in1, OUTPUT);
    pinMode(motorPins[0].in2, OUTPUT);
    pinMode(motorPins[1].in1, OUTPUT);
    pinMode(motorPins[1].in2, OUTPUT);
    
    // acertar o Timer1 para os nossos propósitos
    TCCR1A = _BV(WGM10);                              // WGM1 = 5: Fast PWM 8-bit
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);     // CS1 = 3: prescaler de 64. Resultado: mesma frequência do Timer0
}

// O esquema de fase-magnitude para o controle do
// motor é definido assim: para o fazer o motor
// girar em um sentido, aplica-se PWM em um pino,
// enquanto o outro é mantido LOW. Para o motor
// girar no outro sentido, invertem-se os pinos
// do PWM e do LOW.
// Essa função aceita passos no intervalo
// [-100,+100]

void outSetMotorPower(char motor, int step)
{
    if (step > 100) step = 100;
    else if (step < -100) step = -100;

    if (step > 7)
    {
        analogWrite(motorPins[motor].in1, map(step, 0, 100, 0, MOTOR_MAX_POWER));
        digitalWrite(motorPins[motor].in2, LOW);
    }
    else if (step < -7)
    {
        digitalWrite(motorPins[motor].in1, LOW);
        analogWrite(motorPins[motor].in2, map(-step, 0, 100, 0, MOTOR_MAX_POWER));
    }
    else
    {
        digitalWrite(motorPins[motor].in1, LOW);
        digitalWrite(motorPins[motor].in2, LOW);
    }
}

void outSetLed(char led, bool val)
{
    digitalWrite(leds[led], val ? HIGH : LOW);
}



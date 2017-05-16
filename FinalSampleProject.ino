#include <Filters.h>

#include "Header.hpp"

void setup()
{
    Serial.begin(9600);
  
    inSetup();
    outSetup();

    pinMode(A1, INPUT);
    pinMode(A2, INPUT);
}

void loop()
{
    static unsigned long oldMillis = millis();
    unsigned long cur = millis();
    static bool val = false;
    
    inLoop();
    
    if (oldMillis / 20 != cur / 20)
    {
        val = !val;
        outSetLed(0, val);

        Serial.print(inGetReceptorReadings(0));
        Serial.print(' ');
        Serial.print(inGetReceptorReadings(1));
        Serial.print(' ');
        Serial.print(inGetReceptorReadings(2));
        Serial.println();
    }

    int y = inGetReceptorReadings(0);
    int x = inGetReceptorReadings(1);
    if (inGetReceptorReadings(2) > 0) x = -x;
 
    outSetMotorPower(0, y-x);
    outSetMotorPower(1, y+x);
    
    oldMillis = cur;
}


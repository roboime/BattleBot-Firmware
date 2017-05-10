#include <Filters.h>

#include "Header.hpp"

void setup()
{
    Serial.begin(9600);
  
    inSetup();
    outSetup();
}

void loop()
{
    static unsigned long oldMillis = millis();
    unsigned long cur = millis();
    static int val = 0;
    
    inLoop();
    
    if (oldMillis / 20 != cur / 20)
    {
        Serial.print(inGetReceptorReadings(0));
        Serial.print(' ');
        Serial.print(inGetReceptorReadings(1));
        Serial.print(' ');
        Serial.println(inGetReceptorReadings(2));
        
        val ^= 255;
        outSetLed(val);
    }

    int x = inGetReceptorReadings(0);
    int y = inGetReceptorReadings(1);

    bool xInv = inGetReceptorReadings(2) > 0;
    bool isRight = dipSwitch(0);

    outSetMotorPower(xInv == isRight ? y-x : y+x);
    
    oldMillis = cur;
}


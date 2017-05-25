#include "Header.hpp"
#include <avr/wdt.h>

void setup()
{
    Serial.begin(9600);
  
    inSetup();
    outSetup();
}

void loop()
{
    static unsigned long oldTicks = ticks();
    unsigned long cur = ticks();
    static bool val = false;
    
    inLoop();
    
    if (oldTicks / 6250 != cur / 6250)
    {
        Serial.print(inGetSpeedLeft());
        Serial.print(' ');
        Serial.print(inGetSpeedRight());
        Serial.println();

        //Serial.print(inGetReceptorReadings(0));
        //Serial.print(' ');
        //Serial.print(inGetReceptorReadings(1));
        //Serial.print(' ');
        //Serial.print(inGetReceptorReadings(2));
        //Serial.println();
    }

    int y = inGetReceptorReadings(1);
    int x = inGetReceptorReadings(0);
    if (inGetReceptorReadings(2) > 0) x = -x;
 
    outSetMotorPower(0, y-x);
    outSetMotorPower(1, y+x);

    oldTicks = cur;
}


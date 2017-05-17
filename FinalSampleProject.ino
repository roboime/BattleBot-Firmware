#include <Filters.h>

#include "Header.hpp"

void setup()
{
    Serial.begin(9600);
  
    inSetup();
    outSetup();

    pinMode(A1, OUTPUT);
}

void loop()
{
    static unsigned long oldMillis = millis();
    unsigned long cur = millis();
    static bool val = false;
    
    inLoop();
    
    if (oldMillis / 20 != cur / 20)
    {
        Serial.print(inGetSpeedRight());
        Serial.println();
    }

    int y = inGetReceptorReadings(0);
    int x = inGetReceptorReadings(1);
    if (inGetReceptorReadings(2) > 0) x = -x;
 
    outSetMotorPower(0, y-x);
    outSetMotorPower(1, y+x);

    oldMillis = cur;
}


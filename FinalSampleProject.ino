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
        Serial.println(inGetReceptorReadings(1));
    }

    int x = inGetReceptorReadings(0);
    int y = inGetReceptorReadings(1);

    outSetMotorPower(0, y-x);
    outSetMotorPower(1, y+x);
    
    oldMillis = cur;
}

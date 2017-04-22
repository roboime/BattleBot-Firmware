#include "Header.hpp"

void setup()
{
    Serial.begin(9600);
  
    inSetup();
    outSetup();
    outSetMotorEnabled(true);
}

void loop()
{
    static unsigned long oldMillis = millis();
    unsigned long cur = millis();
    static int val = 0;
    
    inLoop();
    
    if (oldMillis / 20 != cur / 20)
    {
        Serial.println(inGetSpeed());
        val ^= 255;
        outSetLed(val);
    }

    int pot = analogRead(A2);
    outSetMotorPower(map(pot, 0, 1023, -100, 100));
    
    oldMillis = cur;
}

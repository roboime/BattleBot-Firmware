//////////////////////////////////////////////////
// Header.hpp                                   //
//////////////////////////////////////////////////
// Contém as principais definições e variáveis  //
// usadas pelos módulos do código.              //
//////////////////////////////////////////////////

// Módulo input
void inSetup();
void inLoop();
int inGetReceptorReadings(int channel);
unsigned long inGetSpeed();
bool dipSwitch(int port);

// Módulo output
void outSetup();
void outSetLed(char val);
void outSetMotorPower(int step);



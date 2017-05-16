//////////////////////////////////////////////////
// Header.hpp                                   //
//////////////////////////////////////////////////
// Contém as principais definições e variáveis  //
// usadas pelos módulos do código.              //
//////////////////////////////////////////////////

// Módulo input
void inSetup();
void inLoop();
int inGetReceptorReadings(char channel);
int inGetSpeedLeft();
int inGetSpeedRight();
bool dipSwitch(char port);

// Módulo output
void outSetup();
void outSetMotorPower(char motor, int step);
void outSetLed(char led, bool val);

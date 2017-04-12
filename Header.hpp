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
int inGetSpeed();
bool inSignalLost();
bool dipSwitch(int port);

// Módulo output
void outSetup();
void outSetLed(char val);
void outSetMotorEnabled(bool val);
void outSetMotorPower(char step);


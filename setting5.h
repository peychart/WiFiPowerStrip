//Ajust the following:

#define MEMORYLEAKS
uint8_t ResetConfig =      1;     //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME    "ESP8266"
#define DEFAULTWIFIPASS    "defaultPassword"
#define WIFISTADELAYRETRY  30000L
#define WIFIAPDELAYRETRY   300000L
#define MAXWIFIRETRY       2
#define SSIDCount()        3
// Restore output values after a reboot:
#define RESTO_VALUES       false
#define REVERSE_OUTPUT     false
#define inputCount()       3
#define outputCount()      5
//String  outputName[outputCount()] = {"Yellow", "Orange", "Red", "Green", "Blue", "White"}; //can be change by interface...
//int _outputPin[outputCount()]      = {  D0,       D1,      D2,      D3,     D4,      D8  };
String  outputName[outputCount()] = {"Yellow", "Green", "Orange", "Red", "Blue" }; //can be change by interface...
int     _outputPin[outputCount()] = {  D1,       D2,      D3,      D4,     D0  };
int     _inputPin [inputCount()]  = {  D5,       D6,      D7  };

//If DEBUG is defined (Serial.print), D9 & D10 cannot be used as GPIO!!!
//#define DEBUG


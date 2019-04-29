//Ajust the following:

//#define MEMORYLEAKS
uint8_t ResetConfig =      1;     //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME    "ESP8266"
#define DEFAULTWIFIPASS    "defaultPassword"
#define WIFISTADELAYRETRY  30000L
#define WIFIAPDELAYRETRY   300000L
#define MAXWIFIRETRY       2
#define SSIDCount()        3
// Restore output values after a reboot:
#define RESTO_VALUES       false
#define inputCount()       3
#define outputCount()      8
String  outputName[outputCount()] = {"Yellow", "Green", "Orange", "Red", "Blue", "purple", "pink", "white" }; //can be change by interface...
short   _outputPin[outputCount()] = {   D0,       D1,      D2,      D3,    D4,      D8,      D9,     D10   };
short   _inputPin [inputCount()]  = {   D5,       D6,      D7  };

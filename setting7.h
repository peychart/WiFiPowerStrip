//Ajust the following:

uint8_t ResetConfig =      1;     //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME    "ESP8266"
#define DEFAULTWIFIPASS    "defaultPassword"
#define WIFISTADELAYRETRY  30000L
#define WIFIAPDELAYRETRY   300000L
#define MAXWIFIRETRY       2
#define SSIDCount()        3
// Restore output values after a reboot:
#define RESTO_VALUES       false
#define REVERSE_OUTPUT     true
#define inputCount()       3
#define outputCount()      7
String         outputName[outputCount()] = {"Yellow", "Green", "Orange", "Red", "Blue", "purple", "white"}; //can be change by interface...
unsigned short _outputPin[outputCount()] = {   D0,       D1,      D2,      D3,    D4,      D8,      D9  };
unsigned short _inputPin [inputCount()]  = {   D5,       D6,      D7  };

//#define NOTIFPROXY "domogateway"
//#define NOTIFPort 8080
#define MEMORYLEAKS
#define DEBUG

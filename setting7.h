//Ajust the following:

uint8_t ResetConfig =         1;     //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME       "ESP8266"
// NO PASSWORD means NO WiFi in code
#define DEFAULTWIFIPASS       "defaultPassword"
#define WIFISTADELAYRETRY     30000L
#define WIFIAPDELAYRETRY      300000L
#define MEMORYLEAKS           10000L
#define MAXWIFIRETRY          2
#define SSIDCount()           3
// Restore output values after a reboot:
#define RESTO_VALUES          false
#define REVERSE_OUTPUT        false
#define inputCount()          3
#define outputCount(n)        (n)
#define allPins               physPins+uartPins
#define physPins              7
#define uartPins              0
static String  outputName[outputCount(allPins)] ={"Yellow", "Green", "Orange", "Red", "Blue", "purple", "white"}; //can be change by interface...
static uint8_t _outputPin[outputCount(physPins)]={   D0,       D1,      D2,      D3,    D4,      D8,      D9  };
static uint8_t _inputPin [inputCount()]         ={   D5,       D6,      D7  };

#define DISABLESWITCHTIMEOUT 3000L
//#define NOTIFPROXY "domogateway"
//#define NOTIFPort   8080

#define DEBUG

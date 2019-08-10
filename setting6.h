//Ajust the following:

uint8_t ResetConfig =         1;     //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME       "ESP8266"
// NO PASSWORD means NO WiFi in code (Slave modules)
#define DEFAULTWIFIPASS       "defaultPassword"
#define WIFISTADELAYRETRY     30000L
#define WIFIAPDELAYRETRY      300000L
//#define MEMORYLEAKS           10000L
#define MAXWIFIRETRY          2
#define SSIDCount()           3
#define RESTO_VALUES_ON_BOOT  false
#define REVERSE_OUTPUT        false
#define DISABLESWITCHTIMEOUT  3000L
#define inputPinsCount()      3
#define outputPinsCount()     6
static String _outputName[maxPinsCount()]   ={"switch1", "switch2", "switch3", "switch4", "switch5", "switch6"};
static uint8_t _outputPin[outputPinsCount()]={   D1,        D2,        D3,        D4,        D0,        D8    };
static uint8_t _inputPin [inputPinsCount()] ={   D5,        D6,        D7   };

#define NOTIFYPROXY           "192.168.0.253"
#define NOTIFYPORT            8081

//#define DEBUG

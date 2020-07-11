//Ajust the following:

#define DEBUG
#define VERSION                "3.0.1"       //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME        "ESP8266"
#define REFRESH_PERIOD          30
//NOTA: no SSID declared (in web interface) will qualify me as a slave candidate...
#define DEFAULTWIFIPASS        "defaultPassword"
#define RESTO_VALUES_ON_BOOT    false
#define REVERSE_OUTPUT          false
#define DEBOUNCE_TIME           25UL                  //(ms)

//Outsourcing of the user interface to 'EXTERN_WEBUI':
//#define EXTERN_WEBUI         "http://webui-domaine-name/"

#define DEFAULT_NTPSOURCE
#ifdef DEFAULT_NTPSOURCE
  #undef  DEFAULT_NTPSOURCE
  #define DEFAULT_NTPSOURCE    "fr.pool.ntp.org"
  #define DEFAULT_TIMEZONE      -10
  #define DEFAULT_DAYLIGHT      false
  #define NTP_INTERVAL          3600                //(s)
#endif

// MQTT definitions:
#define DEFAULT_MQTT_BROKER
#ifdef DEFAULT_MQTT_BROKER
  #undef  DEFAULT_MQTT_BROKER
  #define DEFAULT_MQTT_BROKER    "mosquitto.home.lan"
  #define DEFAULT_MQTT_PORT       1883
  #define DEFAULT_MQTT_IDENT     ""
  #define DEFAULT_MQTT_USER      ""
  #define DEFAULT_MQTT_PWD       ""
  #define DEFAULT_MQTT_OUT_TOPIC "domoticz/out"
  #define DEFAULT_MQTT_IN_TOPIC  "domoticz/in"
#endif

#define LUMIBLOC_RELAY6X_D1MINI

#ifdef LUMIBLOC_RELAY6X_D1MINI
  #define HOLD_TO_DISABLE_TIMER 3UL       //(s)
  static std::vector<short>    _inputPin ={ D5, D6, D7 };             //relay 6x
  static std::vector<short>    _outputPin={ D8, D1, D2, D3, D4, D0 };
//static std::vector<short>    _outputPin={ D8, D1, D2, D3, D4, D0, -1 }; //with a virtual output...
  #define POWER_LED             D0
  #define WIFISTA_LED           D1
#endif

#ifdef LUMIBLOC_SSR_D1MINI
  #define HOLD_TO_DISABLE_TIMER 3UL       //(s)
  static std::vector<short>    _inputPin ={ D5, D6, D7 };             //lumibloc
  static std::vector<short>    _outputPin={ D1, D2, D3, D4, D0, D8 };
#endif

#ifdef SIMPLESWITCH_ESP01
  #define HOLD_TO_DISABLE_TIMER 3UL       //(s)
  static std::vector<short>    _inputPin ={ 2 };             //ESP-01S relay 1x
  static std::vector<short>    _outputPin={ 0 };
#endif

//TUYA3S:
#ifdef POWERSTRIP
  #define HOLD_TO_DISABLE_TIMER 5UL       //(s)
  static std::vector<short>    _inputPin ={ D7 };             //TUYA TYSE3S Power Strip 3x
  static std::vector<short>    _outputPin={ D6, D5, D8, D3, D2 };
  #define POWER_LED             D0
  #define BLINKING_POWERLED     1000UL,0UL
  #define WIFISTA_LED           D1
  #define BLINKING_WIFILED      5000UL,250UL
#endif

//TUYA3S:
#ifdef POWERSTRIP2
  #define HOLD_TO_DISABLE_TIMER 5UL       //(s)
  static std::vector<short>    _inputPin ={ D7 };             //TUYA TYSE3S Power Strip 3x
  static std::vector<short>    _outputPin={ D1, D2, D3 };
//#define POWER_LED             D0
//#define BLINKING_POWERLED     1000UL,0UL
//#define WIFISTA_LED           D1
//#define BLINKING_WIFILED      5000UL,250UL
#endif

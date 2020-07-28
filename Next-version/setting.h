#ifndef HEADER_FB360A2A9706121
#define HEADER_FB360A2A9706121

//Ajust the following:

#define DEBUG
#define VERSION                "3.0.1"                //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME        "ESP8266"
#define REFRESH_PERIOD          30
//NOTA: no SSID declared (in web interface) will qualify me as a slave candidate...
#define DEFAULTWIFIPASS        "defaultPassword"
#define DEBOUNCE_TIME           25UL                  //(ms)
#define HOLD_TO_DISABLE_TIMER   3UL                   //(s)
#define CMD_COMPLETED_TIMER     1UL                   //(s)

//Outsourcing of the user interface to 'EXTERN_WEBUI':
//#define EXTERN_WEBUI         "http://webui-domaine-name/"

//HTML SCHEMA:
#define ROUTE_VERSION            "/version"
#define ROUTE_HOSTNAME           "/hostname"
#define ROUTE_CHIP_IDENT         "/ident"
#define ROUTE_PIN_STATE          "/state"
#define ROUTE_PIN_SWITCH         "/switch"
#define ROUTE_PIN_TIMEOUT        "/timeout"
#define ROUTE_PIN_NAME           "/name"
#define ROUTE_PIN_REVERSE        "/reverse"
#define ROUTE_PIN_HIDDEN         "/hidden"
#define ROUTE_NTP_SOURCE         "/ntpSource"
#define ROUTE_NTP_ZONE           "/ntpZone"
#define ROUTE_NTP_DAYLIGHT       "/ntpDaylight"
#define ROUTE_MQTT_BROKER        "/mqttBroker"
#define ROUTE_MQTT_PORT          "/mqttPort"
#define ROUTE_MQTT_IDENT         "/mqttIdent"
#define ROUTE_MQTT_USER          "/mqttUser"
#define ROUTE_MQTT_PWD           "/mqttPassword"
#define ROUTE_MQTT_INTOPIC       "/mqttOutputTopic"
#define ROUTE_MQTT_OUTOPIC       "/mqttInputTopic"
#define ROUTE_MQTT_SCHEMA        "/mqttSchema"
#define ROUTE_HTML_CODE          "/html"
#define ROUTE_REBOOT             "/restart"

// MQTT SCHEMA:
#define DEFAULT_MQTT_BROKER      "mosquitto.home.lan"
#ifdef  DEFAULT_MQTT_BROKER
  #define DEFAULT_MQTT_PORT       1883
  #define DEFAULT_MQTT_IDENT     ""
  #define DEFAULT_MQTT_USER      ""
  #define DEFAULT_MQTT_PWD       ""
  #define DEFAULT_MQTT_OUT_TOPIC  ("/home-assistant" + (String(DEFAULT_MQTT_IDENT).length() ?String(String(DEFAULT_MQTT_IDENT)+"/") :String("")) + "/light/" + String(ESP.getChipId(),DEC) + "/").c_str()
  #define DEFAULT_MQTT_IN_TOPIC   (String(ESP.getChipId())  + "/").c_str()
  #define MQTT_SCHEMA(i)          std::map<std::string,untyped>{                                                  \
                                    {"state_topic"  , DEFAULT_MQTT_OUT_TOPIC + toString(i) + ROUTE_PIN_STATE  },  \
                                    {"command_topic", DEFAULT_MQTT_OUT_TOPIC + toString(i) + ROUTE_PIN_SWITCH },  \
                                    {"payload_on"   , "on"  },  /* "toggle" and uppercases are allowed */         \
                                    {"payload_off"  , "off" },                                                    \
                                    {"optimistic"   , false }                                                     \
                                  }
#endif

//#define DEFAULT_NTPSOURCE      "fr.pool.ntp.org"
#ifdef DEFAULT_NTPSOURCE
  #define DEFAULT_TIMEZONE      -10
  #define DEFAULT_DAYLIGHT      false
  #define NTP_INTERVAL          3600                //(s)
#endif

#ifdef _MAIN_

#ifdef DEBUG
  WiFiServer                      telnetServer(23);
  WiFiClient                      telnetClient;
#endif

#define LUMIBLOC_RELAY6X_D1MINI

#ifdef LUMIBLOC_RELAY6X_D1MINI
const std::vector<std::string> INPUT_CONFIG={                                                 //inputs 6x
                              "{\"gpio\": D5, \"name\": \"input1\", \"mode\": INPUT_PULLUP}"
                             ,"{\"gpio\": D6, \"name\": \"input2\", \"mode\": INPUT_PULLUP}"
                             ,"{\"gpio\": D7, \"name\": \"input3\", \"mode\": INPUT_PULLUP}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //relay 6x
                              "{\"gpio\": D8, \"name\": \"Switch1\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D1, \"name\": \"Switch2\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D2, \"name\": \"Switch3\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D3, \"name\": \"Switch4\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D4, \"name\": \"Switch5\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D0, \"name\": \"Switch6\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
};
#endif

#ifdef LUMIBLOC_SSR_D1MINI
const std::vector<std::string> INPUT_CONFIG={                                                 //inputs 6x
                              "{\"gpio\": D5, \"name\": \"input1\", \"mode\": INPUT_PULLUP}"
                             ,"{\"gpio\": D6, \"name\": \"input2\", \"mode\": INPUT_PULLUP}"
                             ,"{\"gpio\": D7, \"name\": \"input3\", \"mode\": INPUT_PULLUP}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //lumibloc
                              "{\"gpio\": D1, \"name\": \"Switch1\", \"reverse\": true, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D2, \"name\": \"Switch2\", \"reverse\": true, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D3, \"name\": \"Switch3\", \"reverse\": true, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D4, \"name\": \"Switch4\", \"reverse\": true, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D0, \"name\": \"Switch5\", \"reverse\": true, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D8, \"name\": \"Switch6\", \"reverse\": true, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
};
#endif

#ifdef SIMPLESWITCH_ESP01
const std::vector<std::string> INPUT_CONFIG={                                                //ESP-01S relay 1x
                              "{\"gpio\": 2, \"name\": \"input1\", \"mode\": INPUT_PULLUP}"
};
const std::vector<std::string> OUTPUT_CONFIG={
                              "{\"gpio\": 0, \"name\": \"Switch1\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
};
#endif

//TUYA3S:
#ifdef POWERSTRIP
  #define POWER_LED
  #define WIFI_STA_LED
const std::vector<std::string> INPUT_CONFIG={                                                 //input 1x
                              "{\"gpio\": D7, \"name\": \"input1\", \"mode\": INPUT_PULLUP}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //TUYA TYSE3S Power Strip 3x
                              "{\"gpio\": D6, \"name\": \"Switch1\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D5, \"name\": \"Switch2\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D8, \"name\": \"Switch3\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D3, \"name\": \"Switch4\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D2, \"name\": \"Switch5\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
#ifdef POWER_LED
                             ,"{\"gpio\": D0, \"name\": \"PowerLed\", \"mode\": OUTPUT, \"display\": false, \"blinking\": true, \"blinkup\": 1000, \"blinkdown\": 0, \"state\": true}"
#endif
#ifdef WIFI_STA_LED
                             ,"{\"gpio\": D1, \"name\": \"PowerLed\", \"mode\": OUTPUT, \"display\": false, \"blinking\": true, \"blinkup\": 5000, \"blinkdown\": 250, \"state\": true}"
#endif
};
#endif

//TUYA3S:
#ifdef POWERSTRIP2
  #define POWER_LED
  #define WIFI_STA_LED
const std::vector<std::string> INPUT_CONFIG={                                                 //input 1x
                              "{\"gpio\": D7, \"name\": \"input1\", \"mode\": INPUT_PULLUP}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //TUYA TYSE3S Power Strip 3x
                              "{\"gpio\": D1, \"name\": \"Switch1\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D2, \"name\": \"Switch2\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
                             ,"{\"gpio\": D3, \"name\": \"Switch3\", \"reverse\": false, \"timeout\": -1, \"mode\": OUTPUT, \"state\": false}"
#ifdef POWER_LED
                             ,"{\"gpio\": D0, \"name\": \"PowerLed\", \"mode\": OUTPUT, \"display\": false, \"blinking\": true, \"blinkup\": 1000, \"blinkdown\": 0, \"state\": true}"
#endif
#ifdef WIFI_STA_LED
                             ,"{\"gpio\": D1, \"name\": \"PowerLed\", \"mode\": OUTPUT, \"display\": false, \"blinking\": true, \"blinkup\": 5000, \"blinkdown\": 250, \"state\": true}"
#endif
};
#endif


#endif
#endif

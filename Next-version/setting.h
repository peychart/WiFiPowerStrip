/* WiFiPowerStrip C++ for ESP8266 (Version 3.0.0 - 2020/07)
    <https://github.com/peychart/WiFiPowerStrip-cpp>

    Copyright (C) 2020  -  peychart

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this program.
    If not, see <http://www.gnu.org/licenses/>.

    Details of this licence are available online under:
                        http://www.gnu.org/licenses/gpl-3.0.html
*/
#ifndef HEADER_FB360A2A9706121
#define HEADER_FB360A2A9706121

//Ajust the following:

#define DEBUG
//#define ALLOW_TELNET_DEBUG

#define VERSION                  "3.0.0"              //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME          "ESP8266"
//NOTA: no SSID declared (in web interface) will qualify me as a slave candidate...
#define DEFAULTWIFIPASS          "defaultPassword"

#define DEBOUNCE_TIME             25UL                //(ms) <- One switches treatments...
#define HOLD_TO_DISABLE_TIMER     3UL                 //(s)  <- One switches treatments...
#define CMD_COMPLETED_TIMER       1UL                 //(s)  <- One switch/multi-outputs treatment...

//Outsourcing of the user interface to 'EXTERN_WEBUI':
//#define EXTERN_WEBUI         "http://webui-domaine-name/"

//HTML SCHEMA:
#define ROUTE_VERSION            "version"
#define ROUTE_HOSTNAME           "hostname"
#define ROUTE__DEFAULT_HOSTNAME  "defHName"
#define ROUTE__DEFAULT_PASSWORD  "defWFPwd"
#define ROUTE_CHIP_IDENT         "ident"
#define ROUTE_UPTIME             "uptime"
#define ROUTE_MAC_ADDR           "macAddr"
#define ROUTE_IP_ADDR            "ip"
#define ROUTE_PIN_GPIO           "gpio"
#define ROUTE_PIN_NAME           "name"
#define ROUTE_PIN_STATE          "state"
#define ROUTE_PIN_SWITCH         "switch"
#define ROUTE_PIN_VALUE          "timeout"
#define ROUTE_PIN_MODE           "mode"
#define ROUTE_PIN_REVERSE        "reverse"
#define ROUTE_PIN_HIDDEN         "hidden"
#define ROUTE_PIN_BLINKING       "blinking"
#define ROUTE_PIN_BLINKING_UP    "blinkingUp"
#define ROUTE_PIN_BLINKING_DOWN  "blinkingDown"
#define ROUTE_WIFI_SSID          "ssid"
#define ROUTE_WIFI_PWD           "pwd"
#define ROUTE_NTP_SOURCE         "ntpSource"
#define ROUTE_NTP_ZONE           "ntpZone"
#define ROUTE_NTP_DAYLIGHT       "ntpDayLight"
#define ROUTE_MQTT_BROKER        "mqttBroker"
#define ROUTE_MQTT_PORT          "mqttPort"
#define ROUTE_MQTT_IDENT         "mqttIdent"
#define ROUTE_MQTT_USER          "mqttUser"
#define ROUTE_MQTT_PWD           "mqttPwd"
#define ROUTE_MQTT_OUTOPIC       "mqttOuTopic"
#define ROUTE_MQTT_INTOPIC       "mqttInTopic"
#define ROUTE_MQTT_SCHEMA        "mqttSchema"
#define ROUTE_HTML_CODE          "html"
#define ROUTE_RESTART            "restart"
#define ROUTE_RESTORE            "restoreStateOnBoot"

// MQTT SCHEMA:
#define DEFAULT_MQTT_BROKER      "mosquitto.home.lan"
#ifdef  DEFAULT_MQTT_BROKER
  #define DEFAULT_MQTT_PORT       1883
  #define DEFAULT_MQTT_IDENT     ""
  #define DEFAULT_MQTT_USER      ""
  #define DEFAULT_MQTT_PWD       ""
  #define DEFAULT_MQTT_OUTOPIC   ("/home-assistant" + (String(DEFAULT_MQTT_IDENT).length() ?String(String(DEFAULT_MQTT_IDENT)+"/") :String("")) + "/light/" + String(ESP.getChipId(),DEC) + "/").c_str()
  #define DEFAULT_MQTT_INTOPIC   (String(ESP.getChipId(), DEC)  + "/").c_str()

  #define STR(i)                  std::string(String(i,DEC).c_str())
  #define MQTT_SCHEMA(i)          std::map<std::string,untyped>{                                               \
                                    {"state_topic"  , DEFAULT_MQTT_OUTOPIC + STR(i) + "/" + ROUTE_PIN_STATE  } \
                                   ,{"command_topic", DEFAULT_MQTT_OUTOPIC + STR(i) + "/" + ROUTE_PIN_SWITCH } \
                                   ,{"payload_on"   , "on"  }   /* "toggle" and uppercases are allowed */      \
                                   ,{"payload_off"  , "off" }                                                  \
                                   ,{"optimistic"   , false }                                                  \
                                  }
#endif

//#define DEFAULT_NTPSOURCE      "fr.pool.ntp.org"
#ifdef DEFAULT_NTPSOURCE
  #define DEFAULT_TIMEZONE      -10
  #define DEFAULT_DAYLIGHT      false
  #define NTP_INTERVAL          3600                //(s)
#endif

#ifdef _MAIN_

#define LUMIBLOC_RELAY6X_D1MINI

#ifdef LUMIBLOC_RELAY6X_D1MINI

const std::vector<std::string> INPUT_CONFIG={                                     //inputs 6x
/*D5*/                        "{\"14\": {\"" ROUTE_PIN_NAME "\": \"input1\" }}"
/*D6*/                       ,"{\"12\": {\"" ROUTE_PIN_NAME "\": \"input2\" }}"
/*D7*/                       ,"{\"13\": {\"" ROUTE_PIN_NAME "\": \"input3\" }}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                    //relay 6x
/*D8*/                        "{\"15\": { \"" ROUTE_PIN_NAME "\": \"Switch1\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D1*/                       ,"{ \"5\": { \"" ROUTE_PIN_NAME "\": \"Switch2\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D2*/                       ,"{ \"4\": { \"" ROUTE_PIN_NAME "\": \"Switch3\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D3*/                       ,"{ \"0\": { \"" ROUTE_PIN_NAME "\": \"Switch4\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D4*/                       ,"{ \"2\": { \"" ROUTE_PIN_NAME "\": \"Switch5\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D0*/                       ,"{\"16\": { \"" ROUTE_PIN_NAME "\": \"Switch6\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
};
#endif

#ifdef LUMIBLOC_SSR_D1MINI
const std::vector<std::string> INPUT_CONFIG={                                                 //inputs 6x
/*D5*/                        "{\"14\": { \"" ROUTE_PIN_NAME "\": \"input1\" }}"
/*D6*/                       ,"{\"12\": { \"" ROUTE_PIN_NAME "\": \"input2\" }}"
/*D7*/                       ,"{\"13\": { \"" ROUTE_PIN_NAME "\": \"input3\" }}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //lumibloc
/*D1*/                        "{ \"5\": { \"" ROUTE_PIN_NAME "\": \"Switch2\", \"" ROUTE_PIN_REVERSE "\": true, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D2*/                       ,"{ \"4\": { \"" ROUTE_PIN_NAME "\": \"Switch3\", \"" ROUTE_PIN_REVERSE "\": true, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D3*/                       ,"{ \"0\": { \"" ROUTE_PIN_NAME "\": \"Switch4\", \"" ROUTE_PIN_REVERSE "\": true, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D4*/                       ,"{ \"2\": { \"" ROUTE_PIN_NAME "\": \"Switch5\", \"" ROUTE_PIN_REVERSE "\": true, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D0*/                       ,"{\"16\": { \"" ROUTE_PIN_NAME "\": \"Switch6\", \"" ROUTE_PIN_REVERSE "\": true, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D8*/                       ,"{\"15\": { \"" ROUTE_PIN_NAME "\": \"Switch1\", \"" ROUTE_PIN_REVERSE "\": true, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
};
#endif

#ifdef SIMPLESWITCH_ESP01
const std::vector<std::string> INPUT_CONFIG={                                                //ESP-01S relay 1x
                              "{\"2\": { \"" ROUTE_PIN_NAME "\": \"input1\", \"mode\": INPUT_PULLUP }}"
};
const std::vector<std::string> OUTPUT_CONFIG={
                              "{\"0\": { \"" ROUTE_PIN_NAME "\": \"Switch1\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"mode\": OUTPUT, \"" ROUTE_PIN_STATE "\": false }}"
};
#endif

//TUYA3S:
#ifdef POWERSTRIP
  #define POWER_LED
  #define WIFI_STA_LED
const std::vector<std::string> INPUT_CONFIG={                                                 //input 1x
/*D7*/                        "{\"13, \"" ROUTE_PIN_NAME "\": \"input3\"}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //TUYA TYSE3S Power Strip 3x
/*D6*/                        "{\"12\": { \"" ROUTE_PIN_NAME "\": \"Switch2\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D5*/                       ,"{\"14\": { \"" ROUTE_PIN_NAME "\": \"Switch2\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D8*/                       ,"{\"15\": { \"" ROUTE_PIN_NAME "\": \"Switch1\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D3*/                       ,"{ \"0\": { \"" ROUTE_PIN_NAME "\": \"Switch4\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D2*/                       ,"{ \"4\": { \"" ROUTE_PIN_NAME "\": \"Switch3\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
#ifdef POWER_LED
/*D0*/                       ,"{\"16\": { \"" ROUTE_PIN_NAME "\": \"PowerLed\", \"" ROUTE_PIN_HIDDEN "\": true, \"" ROUTE_PIN_BLINKING "\": true, \"" ROUTE_PIN_BLINKING_UP "\": 1000, \"" ROUTE_PIN_BLINKING_DOWN "\": 0, \"" ROUTE_PIN_STATE "\": true }}"
#endif
#ifdef WIFI_STA_LED
/*D1*/                       ,"{ \"5\": { \"" ROUTE_PIN_NAME "\": \"PowerLed\", \"" ROUTE_PIN_HIDDEN "\": true, \"" ROUTE_PIN_BLINKING "\": true, \"" ROUTE_PIN_BLINKING_UP "\": 5000, \"" ROUTE_PIN_BLINKING_DOWN "\": 250, \"" ROUTE_PIN_STATE "\": true }}"
#endif
};
#endif

//TUYA3S:
#ifdef POWERSTRIP2
  #define POWER_LED
  #define WIFI_STA_LED
const std::vector<std::string> INPUT_CONFIG={                                                 //input 1x
/*D7*/                        "{\"13\": { \"" ROUTE_PIN_NAME "\": \"input3\" }}"
};
const std::vector<std::string> OUTPUT_CONFIG={                                                //TUYA TYSE3S Power Strip 3x
/*D1*/                        "{ \"5\": { \"" ROUTE_PIN_NAME "\": \"Switch2\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D2*/                       ,"{ \"4\": { \"" ROUTE_PIN_NAME "\": \"Switch3\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
/*D3*/                       ,"{ \"0\": { \"" ROUTE_PIN_NAME "\": \"Switch4\", \"" ROUTE_PIN_REVERSE "\": false, \"" ROUTE_PIN_VALUE "\": -1, \"" ROUTE_PIN_STATE "\": false }}"
#ifdef POWER_LED
/*D0*/                       ,"{\"16\": { \"" ROUTE_PIN_NAME "\": \"PowerLed\", \"" ROUTE_PIN_HIDDEN "\": true, \"" ROUTE_PIN_BLINKING "\": true, \"" ROUTE_PIN_BLINKING_UP "\": 1000, \"" ROUTE_PIN_BLINKING_DOWN "\": 0, \"" ROUTE_PIN_STATE "\": true }}"
#endif
#ifdef WIFI_STA_LED
/*D1*/                       ,"{ \"5\": { \"" ROUTE_PIN_NAME "\": \"PowerLed\", \"" ROUTE_PIN_HIDDEN "\": true, \"" ROUTE_PIN_BLINKING "\": true, \"" ROUTE_PIN_BLINKING_UP "\": 5000, \"" ROUTE_PIN_BLINKING_DOWN "\": 250, \"" ROUTE_PIN_STATE "\": true }}"
#endif
};
#endif

#endif
#endif

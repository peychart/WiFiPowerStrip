#include "FS.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <uart.h>
#include <vector>
#include <map>

#include "setting.h"   //Can be adjusted according to the project...

#define ulong                     long unsigned int

#define Serial_print(m)          {if(Serial) Serial.print(m);}
#define Serial_printf(m,n)       {if(Serial) Serial.printf(m,n);}
#ifdef DEBUG
//  WiFiServer                      telnetServer(23);
//  WiFiClient                      telnetClient;
  #ifdef DEFAULTWIFIPASS
    #define DEBUG_print(m)       {if(telnetClient && telnetClient.connected()) telnetClient.print(m);    Serial_print(m);}
    #define DEBUG_printf(m,n)    {if(telnetClient && telnetClient.connected()) telnetClient.printf(m,n); Serial_printf(m,n);}
  #else
    #define DEBUG_print(m)        Serial_print(m);
    #define DEBUG_printf(m,n)     Serial_printf(m,n);
  #endif
#else
  #define DEBUG_print(m)          ;
  #define DEBUG_printf(m,n)       ;
#endif

#define isMaster()                ssid[0].length() && !String(DEFAULTWIFIPASS).length()
#define inputCount()             _inputPin.size()

inline bool isNow (ulong v)       {ulong ms(millis()); return((v<ms) && (ms-v)<60000UL);}  //Because of millis() rollover:
inline bool isTimeSynchronized(ulong t=now())   {return(t>-1UL/10UL);}

bool readConfig(bool=true);
void writeConfig();
void reboot();
bool WiFiConnect();
void setupWebServer();

void shiftSSID();
void mySerialEvent();
void treatment(String&);

void addSwitch();
bool addMQTT(ushort, ushort);
bool mqttSend(String);

void setPinName(String, String);
void setAllPinsOnSlave();
void setPin(String, bool);
void setScript(String);

/* WiFiPowerStrip C++ for ESP8266 (Version 3.0.0 - 2020/07)
    <https://github.com/peychart/WiFiPowerStrip>

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
//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/

#define _MAIN_
#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <uart.h>

#include "untyped.h"              //<-- de/serialization object for JSON communications and backups on SD card
#include "WiFiManager.h"          //<-- WiFi connection manager
#include "pins.h"                 //<-- pins object manager with serial pin extension manager
#include "switches.h"             //<-- inputs management
#include "webPage.h"              //<-- definition of a web interface
#ifdef DEFAULT_MQTT_BROKER
  #include "mqtt.h"               //<-- mqtt input/output manager
#endif
#ifdef DEFAULT_NTPSOURCE
  #include "ntp.h"                //<-- time-setting manager
#endif

#include "setting.h"              //<--Can be adjusted according to the project...
#include "debug.h"                //<-- telnet and serial debug traces

WiFiManager                       myWiFi;
pinsMap                           myPins;
switches                          mySwitches(myPins);
volatile bool                     intr(false);
volatile ulong                    rebound_completed(0L);
ESP8266WebServer                  ESPWebServer(80);
#ifdef DEFAULT_MQTT_BROKER
  mqtt                            myMqtt;
#endif
#ifdef DEFAULT_NTPSOURCE
  ntp                             myNTP;
#endif
ESP8266HTTPUpdateServer           httpUpdater;

void reboot() {
  DEBUG_print(F("Restart needed!...\n"));
  myPins.serialSendReboot();
  myPins.mustRestore(true).saveToSD();
  ESP.restart();
}

void onWiFiConnect() {
#ifdef DEBUG
#ifdef ALLOW_TELNET_DEBUG
  telnetServer.begin();
  telnetServer.setNoDelay(true);
#endif
#endif
}

void onStaConnect() {
  myPins.master(true);

#ifdef WIFI_STA_LED
  myPins(WIFISTA_LED).set(true);
#endif
}

void ifStaConnected() {
#ifdef MQTT_SCHEMA
  static bool configSended(false);
  static byte retry(0);
  if( !configSended ){
    if(!retry--){
      retry=10; configSended=true;
      for(auto &x : myPins)
        if( !(configSended &= myMqtt.send( untyped(MQTT_SCHEMA(x.gpio())).serializeJson(), STR(x.gpio()) + G("/" MQTT_CONFIG_TOPIC))) )
          break;
  } }
#endif
}

#ifdef WIFI_MEMORY_LEAKS
  struct tcp_pcb;
  extern struct tcp_pcb* tcp_tw_pcbs;
  extern "C" void tcp_abort (struct tcp_pcb* pcb);
  inline void tcpCleanup(){while (tcp_tw_pcbs != NULL) tcp_abort(tcp_tw_pcbs);}
#endif

void ifWiFiConnected() {
#ifdef WIFI_MEMORY_LEAKS                            //No bug according to the ESP8266WiFi devs... but the device ended up rebooting!
  ulong m=ESP.getFreeHeap();                        //(on lack of free memory, particularly quickly on DNS failures)
  DEBUG_print(F("FreeMem: ")); DEBUG_print(m); DEBUG_print(F("\n"));
  if( m < WIFI_MEMORY_LEAKS ){
    ESPWebServer.stop(); ESPWebServer.close(); myWiFi.disconnect(1);
    tcpCleanup();
    ESPWebServer.begin();
    DEBUG_print(F("TCP cleanup -> "));
    DEBUG_print(F("FreeMem: ")); DEBUG_print(ESP.getFreeHeap()); DEBUG_print(F("\n"));
  }
#endif

  if( myPins.slave() )
    myWiFi.disconnect();
  else if( !myPins.master() && Serial )
    myPins.serialSendMasterSearch();                //Is there a Master here?...

#ifdef DEFAULT_NTPSOURCE
  myNTP.getTime();
#endif

#ifdef DEBUG
#ifdef ALLOW_TELNET_DEBUG
  if( telnetServer.hasClient() ) {                  //Telnet client connection:
    if (!telnetClient || !telnetClient.connected()) {
      if(telnetClient){
        telnetClient.stop();
        DEBUG_print(F("Telnet Client Stop\n"));
      }telnetClient=telnetServer.available();
      telnetClient.flush();
      DEBUG_print(F("New Telnet client connected...\n"));
      DEBUG_print("ChipID(" + String(ESP.getChipId(), DEC) + ") says to "); DEBUG_print(telnetClient.remoteIP()); DEBUG_print(F(": Hello World, Telnet!\n\n"));
  } }
#endif
#endif
}

void onStaDisconnect() {
#ifdef WIFISTA_LED
  myPins(WIFISTA_LED).set(false);
#endif
}

#ifdef DEFAULT_MQTT_BROKER
std::string Upper( std::string s ) {std::for_each(s.begin(), s.end(), [](char & c){c = ::toupper(c);}); return s;};

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  for(auto &x : myPins)
    if( strcmp( topic, (DEFAULT_MQTT_INTOPIC + STR(x.gpio()) +  G("/" ROUTE_PIN_SWITCH)).c_str() )==0 )
      x.set( Upper(std::string(reinterpret_cast<char*>(payload), length))==Upper(G(PAYLOAD_ON)) );
}
#endif

void onSwitch( void ) {
#ifdef DEFAULT_MQTT_BROKER
  static std::vector<bool> previous;
  byte i(0); for(auto &x : myPins) if(!isVirtual()){
    if(previous.size()<=i)
      previous.push_back(false);
    if(previous[i] != x.isOn()){
      previous[i] = x.isOn();
      myMqtt.send( x.isOn() ?G(PAYLOAD_ON) :G(PAYLOAD_OFF), STR(x.gpio()) + G("/" ROUTE_PIN_STATE) );
  }i++;}
#endif
}

//Gestion des switchs/Switches management
void ICACHE_RAM_ATTR debouncedInterrupt(){if(!intr){intr=true; rebound_completed = millis() + DEBOUNCE_TIME;}}

// ***********************************************************************************************
// **************************************** SETUP ************************************************
std::vector<std::string> pinFlashDef( String s ){ //Allows pins declaration on Flash...
  std::vector<std::string> v;
  untyped u; u( s.c_str() );
  for(auto &x : u.vector())
    v.push_back( x.serializeJson().c_str() );
  return v;
}

void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial.print(F("\n\nChipID(")); Serial.print(ESP.getChipId()); Serial.print(F(") says: Hello World!\n\n"));

  //initialisation des broches /pins init
  for(ushort i(0); i<2; i++){
    myWiFi.version        ( G(VERSION) )
          .onConnect      ( onWiFiConnect )
          .onStaConnect   ( onStaConnect )
          .ifStaConnected ( ifStaConnected )
          .ifConnected    ( ifWiFiConnected )
          .onStaDisconnect( onStaDisconnect )
          .hostname       ( G(DEFAULTHOSTNAME) )
          .setOutputPower ( 17.5 )
          .restoreFromSD  ();
    if( myWiFi.version() != G(VERSION) ){
#ifndef DEBUG
      myWiFi.clear();
#endif
      if( !LittleFS.format() )
        DEBUG_print(F("LittleFS format failed!\n"));
    }else
      break;
  }DEBUG_print(F("WiFi: ")); DEBUG_print(myWiFi.serializeJson().c_str()); DEBUG_print(F("\n"));
//myWiFi.clear().push_back("hello world", "password").saveToSD();  // only for DEBUG...
  myWiFi.saveToSD();
  myWiFi.connect();

  myPins.set( pinFlashDef(OUTPUT_CONFIG) )
  #ifdef POWER_LED
        .set( pinFlashDef(POWER_LED) )
  #endif
  #ifdef WIFI_STA_LED
        .set( pinFlashDef(WIFI_STA_LED) )
  #endif
        .mode(OUTPUT)
        .onPinChange( onSwitch )
        .restoreFromSD(F("out-gpio-"));
  (myPins.mustRestore() ?myPins.set() :myPins.set(false)).mustRestore(false).saveToSD();
  if( myPins.exist(1) || myPins.exist(3) ) Serial.end();
  #ifdef DEBUG
    for(auto &x : myPins) DEBUG_print((x.serializeJson() + G("\n")).c_str());
  #endif

  mySwitches.set( pinFlashDef(INPUT_CONFIG) )
            .mode(INPUT_PULLUP)
            .restoreFromSD(F("in-gpio-"));
  mySwitches.saveToSD();
  mySwitches.init( debouncedInterrupt, FALLING );   //--> input traitement declared...
  if( mySwitches.exist(1) || mySwitches.exist(3) ) Serial.end();
  #ifdef DEBUG
    for(auto &x : mySwitches) DEBUG_print((x.serializeJson() + G("\n")).c_str());
  #endif

  // Servers:
  setupWebServer();                    //--> Webui interface started...
  httpUpdater.setup( &ESPWebServer );  //--> OnTheAir (OTA) updates added...

#ifdef DEFAULT_MQTT_BROKER
  myMqtt.broker       ( G(DEFAULT_MQTT_BROKER) )
        .port         ( DEFAULT_MQTT_PORT )
        .ident        ( String(F(DEFAULT_MQTT_IDENT)).length() ?G(DEFAULT_MQTT_IDENT) :String(ESP.getChipId(), DEC).c_str() )
        .user         ( G(DEFAULT_MQTT_USER) )
        .password     ( G(DEFAULT_MQTT_PWD) )
        .outTopic     ( DEFAULT_MQTT_OUTOPIC )
        .restoreFromSD();
  myMqtt.saveToSD();
  for(auto x:myPins) myMqtt.subscribe( DEFAULT_MQTT_INTOPIC + STR(x.gpio()) + G("/" ROUTE_PIN_SWITCH) );
  myMqtt.setCallback( mqttCallback );
  DEBUG_print(myMqtt.serializeJson().c_str()); DEBUG_print(F("\n"));
#endif

  //NTP service (not used here):
#ifdef DEFAULT_NTPSOURCE
  myNTP.source        ( G(DEFAULT_NTPSOURCE) )
       .zone          ( DEFAULT_TIMEZONE )
       .dayLight      ( DEFAULT_DAYLIGHT )
       .restoreFromSD ();
  myNTP.saveToSD      ();
  myNTP.begin         ();
  DEBUG_print(myNTP.serializeJson().c_str()); DEBUG_print(F("\n"));
#endif
}

// **************************************** LOOP *************************************************
void loop() {
  ESPWebServer.handleClient(); delay(1L);             //WEB server
  myWiFi.loop();                                      //WiFi manager
  mySwitches.inputEvent(intr, rebound_completed);     //Switches management
  myPins.timers();                                    //Timers control for outputs
  myPins.serialEvent();                               //Serial communication for the serial slave management
#ifdef DEFAULT_MQTT_BROKER
  myMqtt.loop();                                      //MQTT manager
#endif
}
// ***********************************************************************************************

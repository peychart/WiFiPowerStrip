/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/untyped-cpp>

    Copyright (C) 2017  -  peychart

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
// https://github.com/peychart 20171021
// Licence: GNU v3
#define _MAIN_
#include <Arduino.h>
#include <string.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <TimeLib.h>
//#include <Ethernet.h>
#include <uart.h>

#include "untyped.h"              //<-- de/serialization object for JSON communications and SD card backups
#include "WiFiManager.h"          //<-- WiFi connection manager
#include "pins.h"                 //<-- pins object manager with serial pin extension manager
#include "switches.h"             //<-- input management
#ifdef DEFAULT_MQTT_BROKER
  #include "mqtt.h"               //<-- mqtt input/output manager
#endif
#ifdef DEFAULT_NTPSOURCE
  #include "ntp.h"                //<-- time-setting manager
#endif
#include "webPage.h"              //<-- definition of a web interface
#include "debug.h"                //<-- telnet and serial debug traces

#include "setting.h"              //<--Can be adjusted according to the project...

//Avoid to change the following:
pinsMap                           myPins;
switches                          mySwitches( myPins );
WiFiManager                       myWiFi;
WiFiClient                        ethClient;
mqtt                              myMqtt(ethClient);
#ifdef DEFAULT_NTPSOURCE
  ntp                             myNTP;
#endif
volatile bool                     intr(false);
volatile ulong                    rebound_completed(0L);
ESP8266WebServer                  ESPWebServer(80);
ESP8266HTTPUpdateServer           httpUpdater;

#ifdef DEBUG
  WiFiServer                      telnetServer(23);
  WiFiClient                      telnetClient;
#endif

void reboot(){
  DEBUG_print("Restart needed!...\n");
  myPins.serialSendReboot(); delay(1500L);
  myPins.mustRestore(true).saveToSD();
  ESP.restart();
}

void mqttSendConfig( void ) {
#ifdef DEFAULT_MQTT_BROKER
  for(auto x : myPins) myMqtt.send( untyped(MQTT_SCHEMA(x.gpio())).serializeJson(), "sendConfig" );
#endif
}

void onWiFiConnect() {
  myPins.master(true);

}

void onStaConnect() {
  #ifdef WIFI_STA_LED
    myPins.at(WIFISTA_LED).set(true);
  #endif
  mqttSendConfig();
}

void onStaDisconnect() {
#ifdef WIFISTA_LED
  myPins.at(WIFISTA_LED).set(false);
#endif
}

void ifWiFiConnected() {
#ifdef DEFAULT_NTPSOURCE
  myNTP.getTime();
#endif

  if( myPins.slave() )
    myWiFi.disconnect();
  else if( Serial && !myPins.master() && !myPins.slave() )
    myPins.serialSendMasterSearch();                    //Is there a Master here?...
}

bool isNumeric( std::string s ) {
  for(auto x : s)
    if(!isdigit(x)) return false;
  return true;
}

#ifdef DEFAULT_MQTT_BROKER
void mqttPayloadParse( untyped &msg, pin p=pin() ) { //<-- MQTT inputs parser...
  for(auto x : msg.map())
    if ( "/"+x.first == ROUTE_HOSTNAME ) {            // { "hostname": "The new host name" }
      myWiFi.hostname( x.second.c_str() );                                                    myWiFi.saveToSD();
    }else if( x.first == "ssid" ) {                   // { "ssid": {"id", "pwd"} }
      for(auto id : x.second.map()) myWiFi.push_back( id.first.c_str(), id.second.c_str() );  myWiFi.saveToSD();
#ifdef DEFAULT_NTPSOURCE
    }else if( "/"+x.first == ROUTE_NTP_SOURCE ) {     // { "ntpSource": "fr.pool.ntp.org" }
      myNTP.source( x.second.c_str() );                                                       myNTP.saveToSD();
    }else if( "/"+x.first == ROUTE_NTP_ZONE ) {       // { "ntpZone": 10 }
      myNTP.zone( x.second );                                                                 myNTP.saveToSD();
    }else if( "/"+x.first == ROUTE_NTP_DAYLIGHT ) {   // { "ntpDayLight": false }
      myNTP.dayLight( x.second );                                                             myNTP.saveToSD();
#endif
    }else if( "/"+x.first == ROUTE_REBOOT ) {         // reboot
      reboot();
    }else if( isNumeric(x.first) && myPins.exist( atoi(x.first.c_str()) ) ) { // it's a pin config...
      untyped u(x.second.at( atoi(x.first.c_str()) ));
      if( u.map().size() )                            // { "16": { "switch": "ON", "timeout": 3600, "name": "switch0", "reverse": true, "hidden": false } }
        mqttPayloadParse( u, myPins.at( atoi(x.first.c_str()) ) );
      else if( Upper(u.c_str()) == "ON" )             // { "16": "ON" }
        myPins.at( atoi(x.first.c_str()) ).set( true );
      else if( Upper(u.c_str()) == "OFF" )            // { "16": "OFF" }
        myPins.at( atoi(x.first.c_str()) ).set( false );
    }else if( "/"+x.first == ROUTE_PIN_SWITCH ) {     // set the pin output
      p.set( x.second );
    }else if( "/"+x.first == ROUTE_PIN_TIMEOUT ) {    // set the pin timeout
      p.timeout( x.second * 1000UL );                                                         p.saveToSD();
    }else if( "/"+x.first == ROUTE_PIN_NAME ) {       // set the pin name
      p.name( x.second.c_str() );                                                             p.saveToSD();
    }else if( "/"+x.first == ROUTE_PIN_REVERSE ) {    // set the pin reverse
      p.reverse( x.second.value<bool>() );                                                    p.saveToSD();
    }else if( "/"+x.first == ROUTE_PIN_HIDDEN ) {     // set the pin display
      p.display( !x.second.value<bool>() );                                                   p.saveToSD();
    }
}void mqttCallback(char* topic, byte* payload, unsigned int length) {
  untyped msg; msg.deserializeJson( (char*)payload );
  DEBUG_print( "Message arrived on topic: " + String(topic) + "\n" );
  DEBUG_print( String(msg.serializeJson().c_str()) + "\n" );

  mqttPayloadParse( msg );
}
#endif

//Gestion des switchs/Switches management
void ICACHE_RAM_ATTR debouncedInterrupt(){if(!intr){intr=true; rebound_completed = millis() + DEBOUNCE_TIME;}}

// ***********************************************************************************************
// **************************************** SETUP ************************************************
void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial.print("\n\nChipID(" + String(ESP.getChipId(), DEC) + ") says: Hello World!\n\n");

  //initialisation des broches /pins init
  myWiFi.onConnect      ( onWiFiConnect )
        .onStaConnect   ( onStaConnect )
        .ifConnected    ( ifWiFiConnected )
        .onStaDisconnect( onStaDisconnect )
        .onMemoryLeak   ( reboot )
        .hostname       ( DEFAULTHOSTNAME )
        .connect();

  myPins   ( OUTPUT_CONFIG ).restoreFromSD();
  if( myPins.exist(1) || myPins.exist(3) )
    Serial.end();

  mySwitches( INPUT_CONFIG  ).restoreFromSD();
  mySwitches.init( debouncedInterrupt, FALLING );   //--> input traitement declared...

  // Servers:
  setupWebServer();                    //--> Webui interface started...
  httpUpdater.setup( &ESPWebServer );  //--> OnTheAir (OTA) updates added...

  myMqtt.broker       ( DEFAULT_MQTT_BROKER )
        .port         ( DEFAULT_MQTT_PORT )
        .ident        ( DEFAULT_MQTT_IDENT )
        .user         ( DEFAULT_MQTT_USER )
        .password     ( DEFAULT_MQTT_PWD )
        .inputTopic   ( DEFAULT_MQTT_IN_TOPIC )
        .outputTopic  ( DEFAULT_MQTT_OUT_TOPIC )
        .restoreFromSD();
  myMqtt.setCallback( mqttCallback );

  //NTP service (not used here):
#ifdef DEFAULT_NTPSOURCE
  myNTP.source        ( DEFAULT_NTPSOURCE )
       .zone          ( DEFAULT_TIMEZONE )
       .dayLight      ( DEFAULT_DAYLIGHT )
       .restoreFromSD ();
  myNTP.begin         ();
#endif

//myWiFi.push_back("hello world", "");  // only for DEBUG...
}

// **************************************** LOOP *************************************************
void loop() {
  ESPWebServer.handleClient(); delay(1L);             //WEB server
  myWiFi.loop();                                      //WiFi manager
  mySwitches.event(intr, rebound_completed);          //Switches management
  myPins.timers();                                    //Timers control for outputs
  myPins.serialEvent();                               //Serial communication for the serial slave management
  myMqtt.loop();                                      //MQTT manager
}
// ***********************************************************************************************

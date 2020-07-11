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

#include "untyped.h"
#include "WiFiManager.h"
#include "pins.h"
#include "mqtt.h"
#include "ntp.h"
#include "webPage.h"

#include "setting.h"   //Can be adjusted according to the project...
#include "debug.h"

//Avoid to change the following:
pinMap                            myPins;
WiFiManager                       myWiFi;
WiFiClient                        ethClient;
#ifdef DEBUG
  WiFiServer                      telnetServer(23);
  WiFiClient                      telnetClient;
#endif
mqtt                              myMqtt(ethClient);
ntp                               myNTP;
volatile short                    intr(0);
volatile ulong                    rebounds_completed(0L);
ulong                             next_timerDisabled(0L);
bool                              slave(false);
static String                     serialInputString;
ESP8266WebServer                  ESPWebServer(80);
ESP8266HTTPUpdateServer           httpUpdater;

#define isMaster()                myWiFi.ssidCount()
#define isSlave()                 slave

void reboot(){
  if(!isSlave()){
    DEBUG_print("Restart needed!...\n");
    Serial_print("S(.)\n"); delay(1500L);
    myPins.mustRestore(true).saveToSD();
  }ESP.restart();
}

void serialSwitchsTreatment(unsigned int serialBufferLen=serialInputString.length()) {
  if(serialBufferLen>4){
    if( isMaster() ) {
      if(serialInputString.startsWith("M(") && serialInputString[3]==')'){          //Setting Master from Slave:
        if(serialInputString[2]=='?'){
//          setAllPinsOnSlave();
          DEBUG_print("Slave detected...\n");
        }else if(serialInputString[4]==':'){
          if(serialInputString[5]=='-'){ ushort i=myPins.outputCount()+serialInputString[2]-'0';
            myPins.at(i).set(myPins.at(i).isOn(), -1L);
            DEBUG_print( ("Timer removed on uart(" + myPins.at(i).name() + ")\n").c_str() );
          }else{ ushort i=myPins.outputCount()+serialInputString[2]-'0';
//            if(i>=myPins.outputCount() && myPins.at(i).name().length()) writeConfig();     //Create and save new serial pin(s)
            myPins.at(i).set(serialBufferLen>5 && serialInputString[5]=='1');
            //if(serialBufferLen>6 && serialInputString[6]==':') unsetTimer(i); else if(outputValue[i]) setTimer(i);
            DEBUG_print( ("Set GPIO uart(" + myPins.at(i).name() + ") to " + (myPins.at(i).isOn() ?"true\n" :"false\n")).c_str() );
        } }
      }else DEBUG_print( "Slave says: " + serialInputString + "\n" );

    }else if( serialInputString.startsWith("S(") && serialInputString[3]==')' ) {   //Setting Slave from Master:
      if( serialInputString[2]=='.' )
        reboot();
      else if( serialInputString[4]==':' && (ushort)(serialInputString[2]-'0')<myPins.outputCount() )
        myPins.at(serialInputString[2]-'0').set( serialInputString[5]=='1' );
      if( !isSlave() ) DEBUG_print("I'm now the Slave.\n");
      isSlave()=true;

    }serialInputString=serialInputString.substring(serialBufferLen);
} }

void mySerialEvent(){char inChar;
  if(Serial) while(Serial.available()){
    serialInputString += (inChar=(char)Serial.read());
    if(inChar=='\n')
      serialSwitchsTreatment();
} }

void onStaConnect() {
  #ifdef WIFISTA_LED
    myPins.at(WIFISTA_LED).set(true);
  #endif

}

void onStaDisconnect() {
  #ifdef WIFISTA_LED
    myPins.at(WIFISTA_LED).set(false);
  #endif
}

void ifConnected() {
  myNTP.getTime();

  if(isSlave())
    myWiFi.disconnect();
  else if(Serial && !isMaster())
    Serial_print("M(?)\n");                            //Is there a Master here?...
}

//Gestion des switchs/Switchs management
void ICACHE_RAM_ATTR debouncedInterrupt(){if(!intr){intr--;rebounds_completed=millis()+DEBOUNCE_TIME;}}

ushort getInputs(uint16_t reg){
  ushort n(0), i(0);
  for(auto x : myPins){
    if(x->inputMode() && (reg&(1<<x->gpio()))==0){
      n+=(1<<i++);
      if(myPins.inputCount()==myPins.outputCount()){
        n=i+1; break;
  } } }
  return n;
}

void setOutput(ushort n){
  DEBUG_print("IO : "); for(ushort i(myPins.inputCount()); i; i--) DEBUG_print(1<<(i-1)); DEBUG_print("\n");
  DEBUG_print("GPI: "); for(ushort i(myPins.inputCount()); i; i--) DEBUG_print((n+1)&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
  if(n<_outputPin.size()){
    if(isNow(next_timerDisabled)) {
      myPins.at(n).set( myPins.at(n).isOn(), -1UL );
      DEBUG_print( ("Timer removed on " + String(_inputPin[n], DEC) + "(" + myPins.at(n).name().c_str() + ")\n").c_str() );
      if(isSlave())
        Serial_print("M(" + String(n, DEC) + "):-1\n");
    }else myPins.at(n).set( !myPins.at(n).isOn() ); if(myPins.at(n).isOff()) myPins.at(n).unsetTimeout();
} }

void interruptTreatment2(){
  static ushort count=0, incr=0;
  if (intr && isNow(rebounds_completed)){ //switch activated...
    ushort n=getInputs(GPI);
    rebounds_completed=millis()+DEBOUNCE_TIME;
    if(!count && !incr){
      incr=n;
      next_timerDisabled=millis()+HOLD_TO_DISABLE_TIMER*1000UL;
    }if(!n)
       count+=incr;
    incr=n;
    if(!incr) DEBUG_print(".");
      intr=0;
  }if(count+incr && isNow(next_timerDisabled)){
    DEBUG_print("\n");
    count+=incr; count--;
    if(getInputs(GPI)){
      myPins.at(count).set(myPins.at(count).isOn(), -1UL);
      DEBUG_print( "Timer removed on " + String(_inputPin[0], DEC) + "(" + myPins.at(count).name().c_str() + ")\n" );
    }else{
      myPins.at(count).set( !myPins.at(count).isOn() ); if(myPins.at(count).isOff()) myPins.at(count).unsetTimeout();
    }count=incr=0;
} }

void interruptTreatment1(){
  if (intr && isNow(rebounds_completed)){ //switch activated...
    ushort n=getInputs(GPI);
    if (intr<0){  //the switch has just been switched.
      rebounds_completed=millis()+DEBOUNCE_TIME;
      next_timerDisabled=millis()+HOLD_TO_DISABLE_TIMER*1000UL;
      intr=n;
      DEBUG_print("\nIO init: "); for(ushort i(myPins.inputCount()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
    }else if(!n || isNow(next_timerDisabled)){ //Switch released...
      setOutput(--intr);
      intr=0;
    }else if(n!=intr){
      intr=0;
      DEBUG_print("\nIO ERROR.\n"); for(ushort i(myPins.inputCount()); i; i--) DEBUG_print(1<<(i-1));
} } }

void interruptTreatment(){
  if ( myPins.inputCount()==myPins.outputCount() )
        interruptTreatment2();
  else  interruptTreatment1();
}

void initPins(){ ushort i;
  //Set restored pins:
  for( auto x: myPins ){
    if( x->inputMode() )
          attachInterrupt(x->gpio(), debouncedInterrupt, FALLING);
    else  if( x->outputMode() && !myPins.mustRestore() ) x->set(false);
  }if( myPins.mustRestore() ) myPins.mustRestore(false).saveToSD();

  //New inputs?
  i=0; for(auto x : _inputPin) if( !myPins.exist(x) ) {
    myPins.push_back(x).name("in"+toString(i++)).mode(INPUT_PULLUP).display(false);
    DEBUG_print("Input pin \""); DEBUG_print(myPins.at(x).name().c_str()); DEBUG_print("\" created...\n");
  }

  //New outputs?
  i=0; for(auto x : _outputPin) if( !myPins.exist(x) ){
    myPins.push_back(x).name( "Switch" + toString(i++) ).mode(OUTPUT).reverse(REVERSE_OUTPUT);
    DEBUG_print("Output pin \""); DEBUG_print(myPins.at(x).name().c_str()); DEBUG_print("\" created...\n");
  }

#ifdef POWER_LED
  if( !myPins.exist(POWER_LED) )
    myPins.push_back(POWER_LED).name("PowerLed").mode(OUTPUT).blinking(true).blinkUpDelay(1000UL).blinkDownDelay(5000UL).set(true);
#endif

#ifdef WIFISTA_LED
  if( !myPins.exist(WIFISTA_LED) )
    myPins.push_back(WIFISTA_LED).name("WiFiLed").mode(OUTPUT).blinking(true).blinkUpDelay(250).blinkDownDelay(1000UL).set(false);
#endif
}

// ***********************************************************************************************
// **************************************** SETUP ************************************************
void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial_print("\n\nChipID(" + String(ESP.getChipId(), DEC) + ") says: Hello World!\n\n");

  //initialisation des broches /pins init
  myWiFi.onStaConnect   ( onStaConnect );
  myWiFi.ifConnected    ( ifConnected );
  myWiFi.onStaDisconnect( onStaDisconnect );
  myWiFi.onMemoryLeak   ( reboot );

//myWiFi.push_back("hello world", "");

  myWiFi.hostname(DEFAULTHOSTNAME).connect();

  initPins();
  if( myPins.exist(1) || myPins.exist(3) )
        Serial.end();
  else  serialInputString.reserve(32);

#ifdef DEFAULT_MQTT_BROKER
  if( myMqtt.broker().empty() && std::string(DEFAULT_MQTT_BROKER).size() )
    myMqtt.broker     ( DEFAULT_MQTT_BROKER )
          .port       ( DEFAULT_MQTT_PORT )
          .ident      ( DEFAULT_MQTT_IDENT )
          .user       ( DEFAULT_MQTT_USER )
          .password   ( DEFAULT_MQTT_PWD )
          .inputTopic ( DEFAULT_MQTT_IN_TOPIC )
          .outputTopic( DEFAULT_MQTT_OUT_TOPIC );
#endif

  // Servers:
  setupWebServer();                    // Webui interface started...
  httpUpdater.setup( &ESPWebServer );  // OnTheAir (OTA) updates added...

  //NTP service:
#ifdef DEFAULT_NTPSOURCE
  myNTP.restoreFromSD();
  if( myNTP.disabled() && std::string(DEFAULT_NTPSOURCE).size() )
    myNTP.source  ( DEFAULT_NTPSOURCE )
         .zone    ( DEFAULT_TIMEZONE )
         .dayLight( DEFAULT_DAYLIGHT )
         .begin();
#endif
}

// **************************************** LOOP *************************************************
void loop() {
  ESPWebServer.handleClient(); delay(1L);

  myWiFi.loop();                    //WiFi watcher
  interruptTreatment();             //Gestion des switchs/Switchs management
  myPins.timers();                  //Timers control

  mySerialEvent();                  //Serial communication for Slave messages traitement

  myMqtt.loop();
}
// ***********************************************************************************************

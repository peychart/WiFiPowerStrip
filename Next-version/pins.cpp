/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/ESP-pins-cpp>

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
#include "pins.h"

namespace Pins
{
  pin::pin(short g)    : _counter(-1UL), _nextBlink(-1UL), _changed(false) {
    operator[](_NAME_)      = "";             // pin name
    operator[](_GPIO_)      = g;              // pin number
    operator[](_MODE_)      = short(INPUT);   // 0: output, else: input
    operator[](_STATE_)     = false;          // 0 XOR reverse: off. 1 XOR reverse: on
    operator[](_REVERSE_)   = false;          // OFF state
    operator[](_HIDDEN_)    = false;
    operator[](_TIMEOUT_)   = -1L;            // ON delay;
    operator[](_BLINKING_)  = false;          // on state ON, blinking value
    operator[](_BLINKUP_)   = 2000UL;         // blinking delay on
    operator[](_BLINKDOWN_) = 5000UL;         // blinking delay off
    operator[](_RESTORE_)   = false;          // must restore state on boot
  }

  bool   pin::isTimeout()                {
    if(!_isActive() )                     return false;
    if(_counter==-1UL )                   return false;
    if(!_isNow(_counter) )                 return false;
    _counter = -1UL;
    return true;
  }

  pin&   pin::timeout( ulong t )        {
    at(_TIMEOUT_) = -1UL;
    if( _isActive() )
      at(_TIMEOUT_) = t;
    return *this;
  }

  void   pin::startTimer( ulong t )       {
    _counter=-1UL;
    if(!_isActive() || isOff()) return;
    _counter=( t==-1UL ?timeout() :t);
    if(_counter!=-1UL) _counter+=millis();
  }

  pin&   pin::set( bool v, ulong timer )   {
    if( outputMode() ){
      stopTimer(); if(v) startTimer(timer);
      if( !isVirtual() ){
            at(_STATE_)=v;
            digitalWrite( gpio(), at(_STATE_).value<bool>() xor at(_REVERSE_).value<bool>() );
      }else serialSendState();
//    mqttSend( serialiseJson(), "Status-changed" );
      if( mustRestore() )
        saveToSD();
      DEBUG_print( "GPIO \"" + String(name().c_str()) + "(" + String( gpio(), DEC ) + ")\" is now "+ (isOn() ?"on" :"off") +".\n" );
    }else{DEBUG_print( "GPIO \"" + String(name().c_str()) + "(" + String( gpio(), DEC ) + ")\" is not an output !...\n" );}
    return *this;
  }

  bool pin::saveToSD() {
    bool ret(false);
    if( !_isActive() || !_changed ) return true;
    if( LittleFS.begin() ){
      std::ostringstream buff;
      File file( LittleFS.open( ("gpio-" + toString( gpio() ) + ".cfg").c_str(), "w" ) );
      if( file ){
            this->serializeJson( buff );
            ret = file.println( buff.str().c_str() );
            file.close();
            DEBUG_print( "gpio-" + String( gpio(), DEC ) + ".cfg writed.\n" );
      }else{DEBUG_print( "Cannot write gpio-" + String( gpio(), DEC ) + ".cfg !...\n" );}
      LittleFS.end();
    }else{
      DEBUG_print("Cannot open SD!...\n");
    }return ret;
  }

  bool pin::restoreFromSD() {
    bool ret(false);
    if( !_isActive() ) return true;
    if( LittleFS.begin() ){
      String buff;
      File file( LittleFS.open( "gpio-" + String( gpio(), DEC ) + ".cfg", "r" ) );
      if( file ){
            if( (buff = file.readStringUntil('\n')).length() )
              ret = !this->deserializeJson( buff.c_str() ).empty();
            file.close();
            DEBUG_print( "gpio-" + String( gpio(), DEC ) + ".cfg restored.\n" );
      }else{DEBUG_print( "Cannot read gpio-" + String( gpio(), DEC ) + ".cfg !...\n" );}
      LittleFS.end();
    }else{
      DEBUG_print("Cannot open SD!...\n");
    }return ret;
  }

// ----------------------- //

  pinsMap::pinsMap() {
    _nullPin.gpio(-32767);
    _serialInputString.reserve(32);
    restoreFromSD();
  }

  pinsMap& pinsMap::restoreFromSD() {
    if( LittleFS.begin() ){
      Dir dir = LittleFS.openDir("/");
      while (dir.next()){
        String f=dir.fileName();
        ushort i=f.indexOf(".cfg"), g=atoi( f.substring(4, i-4).c_str() );
        if( f.substring(0, 4)=="gpio-" && f.substring(i)==".cfg" )
          push_back(g).restoreFromSD();
      }LittleFS.end();
    }return *this;
  }

  pinsMap& pinsMap::remove( size_t g ) {
    size_t i(indexOf(g));
    if( i!=size_t(-1) ){
      if( LittleFS.begin() ){
        String filename( "gpio-" + String( g, DEC ) + ".cfg" );
        if( !LittleFS.exists(filename) || LittleFS.remove(filename) ) {
          erase(begin()+i);
          DEBUG_print( "GPIO \"" + String(at(g).name().c_str()) + "(" + String( g, DEC ) + ")\" is removed.\n" );
        }LittleFS.end();
    } }
    return *this;
  }

  void pinsMap::timers() {
    for(auto x : *this) {
      if ( x.outputMode() ) {

        // Timeout outputs:
        if( x.isOn() ) {
          if( x.isTimeout() ) {
            if( !x.isVirtual() )
                  x.set(false);
            else  Serial_print( "S" + String(-x.gpio()-1, DEC) + ":0\n" );
          // Blinking outputs:
          }else if( x.blinking() && pin::_isNow(x._nextBlink) ) {
            bool v( !(digitalRead(x.gpio()) xor x.reverse()) );
            if( !x.isVirtual() )
                  digitalWrite(x.gpio(), v);
            else {Serial_print( "s" + String(-x.gpio()-1, DEC) + ":" + (v ?"1" :"0") + "\n" );}
            x._nextBlink = millis() + ( v ?x.blinkUpDelay() :x.blinkDownDelay() );
  } } } } }

  bool pinsMap::_serialPinsTreatment( void ) {
    ushort i(2), x;
    if( _serialInputString.length()<3 ) return false;

    if( master() ){ 
      if( _serialInputString[0]!='M' )  return false;
      
      if( _serialInputString[i]==':' || _serialInputString[++i]==':' ){
        if( _serialInputString[1]=='?' ){
          if( _serialInputString[1]=='\n' ){
            _setAllPinsOnSlave();            //--> update de slave
            DEBUG_print("Slave detected...\n");
            return true;                     //--> slave detected
          }else return false;
        }

        if( indexOf( (x=atol(_serialInputString.substring(1,i).c_str())) ) != size_t(-1) ){
          if(_serialInputString[++i]=='-' ){
            if( _serialInputString[++i]=='\n' ){
              at(x).set(at(i).isOn(), -1L);
              DEBUG_print( ("Timer removed on uart(" + at(x).name() + ")\n").c_str() );
            }else return false;
          }else{
            if( _serialInputString[i+1]=='\n' ){
              at(x).set( _serialInputString[i]=='1' );
              DEBUG_print( ("Set GPIO uart(" + at(x).name() + ") to " + (at(x).isOn() ?"true\n" :"false\n")).c_str() );
            }else return false;
          }
        }else return false;
      }else return false;
      return true;
    }

    //I am the slave:
    if( _serialInputString[0]=='S' || _serialInputString[0]=='s' ){
      _slave = true;
      if (_serialInputString[i]==':' || _serialInputString[++i]==':') {
        if( _serialInputString[1]=='.' ){
          if( _serialInputString[1]=='\n' )
                ESP.restart();
          else  return false;
        }

        if( indexOf( (x=atol(_serialInputString.substring(1,i).c_str())) ) != size_t(-1) ){
          if( _serialInputString[++i]=='\n' ){
            bool s( _serialInputString[i]=='1' );
            if( at(x).isOn() != s ){
              at(x).set( s );
              at(x).serialSendState( false );
          } }
          else return false;
        }else return false;
      }else return false;
      return true;
    }

    return false;
  }

  void pinsMap::serialEvent(){ char inChar;
    if(Serial) while(Serial.available()){
      _serialInputString += (inChar=(char)Serial.read());
      if(inChar=='\n'){
        if( !_serialPinsTreatment() ){
          DEBUG_print( "Slave says: " + _serialInputString + "\n" );
        }_serialInputString = _serialInputString.substring( _serialInputString.length() );
  } } }

}

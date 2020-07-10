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
#include "pins.h"

namespace Pins
{
  pin::pin(short g)    : _counter(-1UL), _nextBlink(-1UL), _changed(false) {
    operator[](_NAME_)     = "";              // pin name
    operator[](_GPIO_)     = g;               // pin number
    operator[](_MODE_)     = INPUT;           // 0: output, else: input
    operator[](_STATE_)    = false;           // 0 XOR reverse: off. 1 XOR reverse: on
    operator[](_REVERSE_)  = false;           // OFF state
    operator[](_HIDDEN_)   = true;
    operator[](_TIMEOUT_)  = -1UL;            // ON delay;
    operator[](_BLINKING_) = false;           // on state ON, blinking value
    operator[](_BLINKUP_)  = 2000L;           // blinking delay on
    operator[](_BLINKDOWN_)= 2000L;           // blinking delay off
    operator[](_RESTORE_)  = false;           // must restore state on boot
  }

  bool   pin::isTimeout()                {
    if(!_isActive() )                     return false;
    if(_counter==-1UL )                   return false;
    if(!isNow(_counter) )                 return false;
    _counter = -1UL;
    return true;
  }

  pin&   pin::setTimeout( ulong t)        {
    at(_TIMEOUT_)=-1UL;
    if( _isActive() )
      at(_TIMEOUT_)=t;
    return *this;
  }

  void   pin::startTimer( ulong t )       {
    _counter=-1UL;
    if(!_isActive() || !at(_STATE_)) return;
    _counter=( t==-1UL ?timeout() :t);
    if(_counter!=-1UL) _counter+=millis();
  }

  pin&   pin::set(bool v, ulong timer)   {
    if( outputMode() ) {
      at(_STATE_)=v;
      stopTimer(); if(v) startTimer(timer);
      if( isVirtual() ) {
/*        if( slave() )
              Serial_print( "M(" + String(i, DEC) + "):" + (v ?"1" :"0") + "\n" );
        else  Serial_print( "S(" + String(i-myPins.size(), DEC) + "):" + (v ?"1\n" :"0\n") );*/
      }else  digitalWrite( at(_GPIO_), at(_STATE_).value<bool>() xor !at(_REVERSE_).value<bool>() );
//      mqttSend( serialiseJson(), "Status-changed" );
      if( mustRestore() )
        saveToSD();
      DEBUG_print("GPIO \"" + String(at(_NAME_).c_str()) + "(" + String( gpio(), DEC ) + ")\" is now "+ (isOn() ?"on" :"off") +".\n");
    }else{DEBUG_print("GPIO \"" + String(at(_NAME_).c_str()) + "(" + String( gpio(), DEC ) + ")\" is not an output !...\n");}
    return *this;
  }

  bool pin::saveToSD() {
    bool ret(false);
    if( !_isActive() || !_changed ) return true;
    if( LittleFS.begin() ) {
      std::ostringstream buff;
      File file( LittleFS.open( ("gpio-" + toString(at(_GPIO_)) + ".cfg").c_str(), "w" ) );
      if( file ) {
            this->serializeJson( buff );
            ret = file.println( buff.str().c_str() );
            file.close();
            DEBUG_print("gpio-" + String( gpio(), DEC ) + ".cfg writed.\n");
      }else{DEBUG_print("Cannot write gpio-" + String( gpio(), DEC ) + ".cfg !...\n");}
      LittleFS.end();
    }else{
      DEBUG_print("Cannot open SD!...\n");
    }return ret;
  }

  bool pin::restoreFromSD() {
    bool ret(false);
    if( !_isActive() ) return true;
    if( LittleFS.begin() ) {
      String buff;
      File file( LittleFS.open( "gpio-" + String( gpio(), DEC ) + ".cfg", "r" ) );
      if( file && (buff=file.readStringUntil('\n')).length()
               && (ret=!this->deserializeJson( buff.c_str() ).empty()) ) {
            file.close();
            DEBUG_print("gpio-" + String( gpio(), DEC ) + ".cfg restored.\n");
      }else{DEBUG_print("Cannot read gpio-" + String( gpio(), DEC ) + ".cfg !...\n");}
      LittleFS.end();
    }else{
      DEBUG_print("Cannot open SD!...\n");
    }return ret;
  }

// ----------------------- //

  pinMap::pinMap() {
    nullPin.gpio(-32767);
    restoreFromSD();
  }

  pinMap& pinMap::restoreFromSD() {
    if( LittleFS.begin() ) {
      Dir dir = LittleFS.openDir("/");
      while (dir.next()) {
        String f=dir.fileName();
        ushort i=f.indexOf(".cfg"), g=atoi( f.substring(4, i-4).c_str() );
        if( f.substring(0, 4)=="gpio-" && f.substring(i)==".cfg" )
          push_back(g).restoreFromSD();
      }LittleFS.end();
    }return *this;
  }

  pinMap& pinMap::remove( ushort g ) {
    size_t i(indexOf(g));
    if( i!=size_t(-1) ) {
      if( LittleFS.begin() ) {
        String filename( "gpio-" + String( g, DEC ) + ".cfg" );
        if( !LittleFS.exists(filename) || LittleFS.remove(filename) ) {
          delete operator[](i); erase(begin()+i);
          DEBUG_print("GPIO \"" + String(at(_NAME_).c_str()) + "(" + String( g, DEC ) + ")\" is removed.\n");
        }LittleFS.end();
    } }
    return *this;
  }

  void pinMap::timers() {
    for(auto x : list()){
      if (x->outputMode()) {

        // Timeout outputs:
        if(x->isOn() && x->isTimeout())
          x->set(false);

        // Blinking outputs:
        if(x->isOn() && x->blinking()) {
          if(isNow(x->_nextBlink)) {
            bool state = digitalRead(x->gpio()) xor x->reverse();
            if(state) {
              digitalWrite(x->gpio(), !state);
              x->_nextBlink = millis() + x->blinkDownDelay();
            }else{
              digitalWrite(x->gpio(), state);
              x->_nextBlink = millis() + x->blinkUpDelay();
  } } } } } }

}

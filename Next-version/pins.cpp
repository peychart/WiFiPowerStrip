/* ESP8266-pins-Manager C++ (Version 0.1 - 2020/07)
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
#include "pins.h"

namespace Pins
{
  pin::pin(short g) : _counter(-1UL), _nextBlink(-1UL), _changed(false), _backupPrefix("")
                     ,_on_switch(0),  _on_timeout(0),   _on_blinkup(0),  _on_blinkdown(0) {
    json();
    operator[](ROUTE_PIN_GPIO)          = g;                // pin number
    operator[](ROUTE_PIN_NAME)          = ROUTE_PIN_SWITCH; // pin name
    operator[](ROUTE_PIN_MODE)          = ushort(INPUT);    // 0: output, else: input
    operator[](ROUTE_PIN_STATE)         = false;            // 0 XOR reverse: off. 1 XOR reverse: on
    operator[](ROUTE_PIN_REVERSE)       = false;            // OFF state
    operator[](ROUTE_PIN_HIDDEN)        = false;
    operator[](ROUTE_PIN_VALUE)         = -1L;              // ON delay;
    operator[](ROUTE_PIN_BLINKING)      = false;            // on state ON, blinking value
    operator[](ROUTE_PIN_BLINKING_UP)   = 2000UL;           // blinking delay on
    operator[](ROUTE_PIN_BLINKING_DOWN) = 5000UL;           // blinking delay off
    operator[](ROUTE_RESTORE)           = false;            // must restore state on boot
  }

  void pin::startTimer( ulong t ) {
    _counter=-1UL;
    if( !_isActive() ) return;
    _counter=( t==-1UL ?timeout() :t);
    if(_counter!=-1UL ) _counter += millis();
  }

  pin& pin::set( bool v, ulong timer ) {
    if( outputMode() ){
      if( (at(ROUTE_PIN_STATE)=v) ) startTimer(timer); else stopTimer();
      if( !isVirtual() ){
        if( digitalRead(gpio()) != (isOn() xor reverse()) ){
          digitalWrite( gpio(), isOn() xor reverse() );
          DEBUG_print( "GPIO \"" + String(name().c_str()) + "(" + String( gpio(), DEC ) + ")\" is now "+ (isOn() ?"on" :"off") +".\n" );
      } }
      else _serialSendState();
      if( _on_switch ) (*_on_switch)();
      if( mustRestore() ) saveToSD();
    }else{DEBUG_print( "GPIO(" + String( gpio(), DEC ) + ")\" is not an output !...\n" );}
    return *this;
  }

  bool pin::saveToSD( String prefix ) {
    if( prefix.length() ) _backupPrefix=prefix;
    if( !_changed || !_isActive() ) return true;

    if( LittleFS.begin() ){
      File file( LittleFS.open( _backupPrefix + String( gpio(), DEC ) + ".cfg", "w" ) );
      if( file ){
            _changed = !file.println( this->serializeJson().c_str() );
            file.close();
            DEBUG_print( _backupPrefix + String( gpio(), DEC ) + ".cfg writed.\n" );
      }else{DEBUG_print( "Cannot write " + _backupPrefix + String( gpio(), DEC ) + ".cfg !...\n" );}
      LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return !_changed;
  }

  bool pin::_restoreFromSD( String prefix ) {
    bool ret(false);
    if( prefix.length() ) _backupPrefix=prefix;
    File file( LittleFS.open( _backupPrefix + String( gpio(), DEC ) + ".cfg", "r" ) );
    if( file ){
          ret = !this->deserializeJson( file.readStringUntil('\n').c_str() ).empty();
          file.close();
          DEBUG_print( _backupPrefix + String( gpio(), DEC ) + ".cfg restored.\n" );
    }else{DEBUG_print( "Cannot read " + _backupPrefix + String( gpio(), DEC ) + ".cfg !...\n" );}
    return ret;
  }

  bool pin::restoreFromSD( String prefix ) {
    bool ret(false);
    if( prefix.length() ) _backupPrefix=prefix;
    if( LittleFS.begin() ){
      ret = !(_changed = _restoreFromSD( _backupPrefix ));
      LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return ret;
  }

// ----------------------- //

  pinsMap& pinsMap::set( untyped v ) {
    for(auto &x :v.map()[ROUTE_PIN_GPIO].map()){
      bool modified(false);
      ushort g( atoi(x.first.c_str()) );
      for(auto &y: x.second.map())
        if( !_isInPins(y.first) )
              x.second.map().erase(y.first);
        else  modified|=(y.first != ROUTE_PIN_STATE && at( atoi(x.first.c_str()) ).at( y.first ) != y.second);
      if( x.second.map().size() ){
        push_back( g ).changed( modified ) += x.second;
        if(at(g).outputMode()) set(g);
    } }
    return *this;
  }

  void pinsMap::timers() {
    for(auto &x : *this) if( x.isOn() ){
      if( x.isTimeout() ){
        if( x.outputMode() ) x.set(false);
        if( x._on_timeout ) (*x._on_timeout)();
      }

      if( x.blinking() && pin::_isNow(x._nextBlink) ){
        bool v( !(digitalRead(x.gpio()) xor x.reverse()) );
        if( !x.isVirtual() )
              digitalWrite(x.gpio(), v);
        else {Serial_print( "s" + String(-x.gpio()-1, DEC) + ":" + (v ?"1" :"0") + "\n" );}
        if( v ) {
          x._nextBlink = millis() + x.blinkUpDelay();
          if( x._on_blinkdown ) (*x._on_blinkdown)();
        }else{
          x._nextBlink = millis() + x.blinkDownDelay();
          if( x._on_blinkup ) (*x._on_blinkup)();
  } } } }

  pinsMap& pinsMap::restoreFromSD( String prefix ) {
    if( prefix.length() ) _backupPrefix=prefix;
    if( LittleFS.begin() ){
      Dir dir = LittleFS.openDir("/");
      while (dir.next()){
        String f(dir.fileName());
        short  i(f.indexOf(".cfg"));
        if( i>0 && f.substring(0, _backupPrefix.length())==_backupPrefix )
          push_back( atoi(f.substring(_backupPrefix.length(), i).c_str()) )._restoreFromSD( _backupPrefix );
      }LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return *this;
  }

  pinsMap& pinsMap::remove( ushort g ) {
    if( LittleFS.begin() ){
      for(size_t i(0); i<size(); i++) if( (*this)[i].gpio()==g ){
        String filename( _backupPrefix + String( g, DEC ) + ".cfg" );
        if( !LittleFS.exists(filename) || LittleFS.remove(filename) ){
          erase( begin()+i );
          DEBUG_print( "gpio(" + String( g, DEC ) + ") \"" + at(g).name().c_str() + "\" has been removed.\n" );
        }else{DEBUG_print("Cannot remove \"" + filename + "\" on SD: pin gpio(" + String( g, DEC ) + ") not removed!...\n");}
        LittleFS.end();
        break;
      }else{DEBUG_print("Cannot find gpio(" + String( g, DEC ) + "): not removed!...\n");}
    }else{DEBUG_print("Cannot open SD!...\n");}
    return *this;
  }

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

        if( (x=atol(_serialInputString.substring(1,i).c_str())) < size() ){
          if(_serialInputString[++i]=='-' ){
            if( _serialInputString[++i]=='\n' ){
              (*this)[x].set(at(i).isOn(), -1L);
              DEBUG_print( ("Timer removed on uart(" + (*this)[x].name() + ")\n").c_str() );
            }else return false;
          }else{
            if( _serialInputString[i+1]=='\n' ){
              (*this)[x].set( _serialInputString[i]=='1' );
              DEBUG_print( ("Set GPIO uart(" + (*this)[x].name() + ") to " + ((*this)[x].isOn() ?"true\n" :"false\n")).c_str() );
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

        if( (x=atol(_serialInputString.substring(1,i).c_str())) < size() ){
          if( _serialInputString[++i]=='\n' ){
            bool s( _serialInputString[i]=='1' );
            if( (*this)[x].isOn() != s ){
              (*this)[x].set( s );
              (*this)[x]._serialSendState( false );
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

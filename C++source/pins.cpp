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
  pin::pin(short g) : _counter(-1UL),   _nextBlink(-1UL), _changed(false),  _backupPrefix(""),
                      _on_timeout(0),   _on_blinkup(0),   _on_blinkdown(0), _on_state_change(0) {
    json();
    operator[](G(ROUTE_PIN_GPIO))          = g;                // pin number
    operator[](G(ROUTE_PIN_NAME))          = ROUTE_PIN_CMD;    // pin name
    operator[](G(ROUTE_PIN_MODE))          = byte(INPUT);      // 0: output, else: input
    operator[](G(ROUTE_PIN_STATE))         = false;            // 0 XOR reverse: off. 1 XOR reverse: on
    operator[](G(ROUTE_PIN_REVERSE))       = false;            // OFF state
    operator[](G(ROUTE_PIN_HIDDEN))        = false;
    operator[](G(ROUTE_PIN_VALUE))         = -1L;              // ON delay;
    operator[](G(ROUTE_PIN_ENABLED))       = true;             // ON delay;
    operator[](G(ROUTE_PIN_BLINKING))      = false;            // on state ON, blinking value
    operator[](G(ROUTE_PIN_BLINKING_UP))   = 2000UL;           // blinking delay on
    operator[](G(ROUTE_PIN_BLINKING_DOWN)) = 5000UL;           // blinking delay off
    operator[](G(ROUTE_RESTORE))           = false;            // must restore state on boot
  }

  void pin::startTimer( ulong t ) {
    _counter=-1UL;
    if( !_isActive() ) return;
    _counter=((t==-1UL) ?timeout() :t);
    if(_counter!=-1UL ) _counter += millis();
  }

  pin& pin::set( bool v, ulong timer ) {
    if( outputMode() ){
      if( (at(G(ROUTE_PIN_STATE))=v) ){
        if(isEnabled()) startTimer(timer); else isEnabled(true);
      }else stopTimer();
      if( isVirtual() )
        _serialSendState();
      else if( digitalRead(gpio()) != (isOn() xor reverse()) ){
        digitalWrite( gpio(), isOn() xor reverse() );
        if( _on_state_change ) (*_on_state_change)();
        DEBUG_print(G("GPIO \"") + String(name().c_str()) + G("(") + String(gpio(), DEC) + G(")\" is now ") + (isOn() ?G("on") :G("off")) + G(".\n"));
      }if( mustRestore() ) saveToSD();
    }else{DEBUG_print(G("GPIO(") + String(gpio(), DEC) + G(")\" is not an output !...\n"));}
    return *this;
  }

  bool pin::saveToSD( String prefix ) {
    if( prefix.length() ) _backupPrefix=prefix;
    if( !_changed || !_isActive() ) return true;

    if( LittleFS.begin() ){
      File file( LittleFS.open(_backupPrefix + String( gpio(), DEC ) + G(".cfg"), "w" ));
      if( file ){
            _changed = !file.println( this->serializeJson().c_str() );
            file.close();
            DEBUG_print(_backupPrefix + String(gpio(), DEC) + G(".cfg writed.\n"));
      }else{DEBUG_print(G("Cannot write ") + _backupPrefix + String(gpio(), DEC) + G(".cfg !...\n"));}
      LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return !_changed;
  }

  bool pin::_restoreFromSD( String prefix ) {
    if( prefix.length() ) _backupPrefix=prefix;
    File file( LittleFS.open(_backupPrefix + String(gpio(), DEC) + G(".cfg"), "r" ) );
    if( file ){
          _changed = this->deserializeJson( file.readStringUntil('\n').c_str() ).empty();
          file.close();
          DEBUG_print(_backupPrefix + String(gpio(), DEC) + G(".cfg restored.\n"));
    }else{DEBUG_print(G("Cannot read ") + _backupPrefix + String(gpio(), DEC) + G(".cfg !...\n"));}
    return !_changed;
  }

  bool pin::restoreFromSD( String prefix ) {
    if( prefix.length() ) _backupPrefix=prefix;
    if( LittleFS.begin() ){
      _changed = !_restoreFromSD(_backupPrefix );
      LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return !_changed;
  }

// ----------------------- //

  pinsMap& pinsMap::set( untyped v ) {
    for(auto &x :v.map()[ROUTE_PIN_GPIO].map()){
      bool modified(false);
      ushort g( atoi(x.first.c_str()) );
      for(auto &y: x.second.map())
        if( !_isInPins(y.first) )
              x.second.map().erase(y.first);
        else  modified|=(y.first != ROUTE_PIN_STATE && y.first != ROUTE_PIN_ENABLED && at( atoi(x.first.c_str()) ).at( y.first ) != y.second);
      if( x.second.map().size() ){
        push_back( g ).changed( modified ) += x.second;
        if(at(g).outputMode()) set(g);
    } }
    return *this;
  }

  void pinsMap::timers() {
    for(auto &x : *this) if( x.isOn() ){
      if( x.isTimeout() ){
        x.set(false);
        if( x._on_timeout ) (*x._on_timeout)();
      }

      if( x.blinking() && pin::_isNow(x._nextBlink) ){
        bool v( !(digitalRead(x.gpio()) xor x.reverse()) );
        if( !x.isVirtual() )
              digitalWrite(x.gpio(), v);
        else {Serial_print(G("s") + String(-x.gpio()-1, DEC) + G(":") + (v ?G("1") :G("0")) + G("\n") );}
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
      Dir dir = LittleFS.openDir(F("/"));
      while (dir.next()){
        String f(dir.fileName());
        short  i(f.indexOf(F(".cfg")));
        if( i>0 && f.substring(0,_backupPrefix.length())==_backupPrefix )
          push_back( atoi(f.substring(_backupPrefix.length(), i).c_str()) )._restoreFromSD( _backupPrefix );
      }LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return *this;
  }

  pinsMap& pinsMap::remove( ushort g ) {
    if( LittleFS.begin() ){
      for(size_t i(0); i<size(); i++) if( (*this)[i].gpio()==g ){
        String filename(_backupPrefix + String(g, DEC) + G(".cfg") );
        if( !LittleFS.exists(filename) || LittleFS.remove(filename) ){
          erase( begin()+i );
          DEBUG_print(G("gpio(") + String(g, DEC) + G(") \"") + at(g).name().c_str() + G("\" has been removed.\n"));
        }else{DEBUG_print(G("Cannot remove \"") + filename + G("\" on SD: pin gpio(") + String(g, DEC) + G(") not removed!...\n"));}
        LittleFS.end();
        break;
      }else{DEBUG_print(G("Cannot find gpio(") + String(g, DEC) + G("): not removed!...\n"));}
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return *this;
  }

  bool pinsMap::_setSerialPin() {
    ushort i(_serialInputString.indexOf(F(":"))), gpio(atoi(_serialInputString.substring(1,i++).c_str()));
    if( gpio >= size() ) {
      // if( master() createVirtualPin(); ?...
      return false;
    }

    if(_serialInputString[i]=='-' )
          (*this)[gpio].stopTimer();
    else if(_serialInputString[i]=='1' )
          (*this)[gpio].set( true );
    else  (*this)[gpio].set( false );

    if(_serialInputString[0]=='S' )        // Response to the Master:
      (*this)[gpio]._serialSendState( (*this)[gpio].isOn() );

    return true;
  }

  bool pinsMap::_serialPinsTreatment( void ) {
    ushort i(0);
    if( _serialInputString.length()<4 )                                   return false;

    if( !master() ){                       // From the Master to the Slave:
      if(_serialInputString[i]!='S' && _serialInputString[i]!='s')        return false;
      _slave = true;                       // I'm the Slave...
    }else                                  // From the Slave to the Master:
      if( _serialInputString[i]!='M' )                                    return false;
    if( _serialInputString[++i]!=':' && _serialInputString[++i]!=':' )    return false;

    if( _serialInputString[++i]<'0' || _serialInputString[i]>'9' ){
      if(_serialInputString[i]!='?')                                      return false;
      if( master() ){
            _setAllPinsOnSlave();          //--> update the slave
            DEBUG_print(F("Slave detected...\n"));
      }else ESP.restart();
      return true;
    }

    return _setSerialPin();
  }

  void pinsMap::serialEvent(){ char inChar;
    if(Serial) while(Serial.available()){
      _serialInputString += (inChar=(char)Serial.read());
      if(inChar=='\n'){
        if( !_serialPinsTreatment() ){
          if(master()) {DEBUG_print(G("Slave says: ") + _serialInputString + G("\n"));}
        }_serialInputString = _serialInputString.substring( _serialInputString.length() );
  } } }

}

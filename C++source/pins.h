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
/********************************* Serial pins Management ***********************************
  On the naster, the value of a serial gpio must be : (-1) * ( physicalSlaveGpioValue + 1 ).
  Examples:
    gpio 16 on slave is -17 on it's master,
    gpio  0 on slave is  -1 on it's master...
  Serial codes:
    From the master() to the slave():
      - "Sg:0" --> gpio(g) must be FLASE on slave and new state return
      - "Sg:1" --> gpio(g) must be TRUE  on slave and new state return
      - "sg:0" --> gpio(g) must be FALSE on slave in optimistic mode
      - "sg:1" --> gpio(g) must be TRUE  on slave in optimistic mode
      - "S:??" --> slave() must reboot.
    From the slave() to the master():
      - "M:??" --> ask for a master,
      - "Mg:1" --> the new state of gpio(g) is now: TRUE 
      - "Mg:0" --> the new state of gpio(g) is now: FALSE 
      - "Mg:-" --> the timeout of gpio(g) must be disabled.
********************************************************************************************/
#ifndef HEADER_FB360F1A9606491
#define HEADER_FB360F1A9606491

#include <Arduino.h>
#include <LittleFS.h>
#include "untyped.h"

#include "setting.h"
#include "debug.h"

// Json name attributes:
#ifndef ROUTE_PIN_NAME
  #define ROUTE_PIN_NAME           "name"
  #define ROUTE_PIN_GPIO           "gpio"
  #define ROUTE_PIN_MODE           "mode"
  #define ROUTE_PIN_STATE          "state"
  #define ROUTE_PIN_REVERSE        "reverse"
  #define ROUTE_PIN_HIDDEN         "hidden"
  #define ROUTE_PIN_VALUE          "timeout"
  #define ROUTE_PIN_ENABLED        "enabled"
  #define ROUTE_PIN_BLINKING       "blinking"
  #define ROUTE_PIN_BLINKING_UP    "blinkup"
  #define ROUTE_PIN_BLINKING_DOWN  "blinkdown"
  #define ROUTE_RESTORE            "restoreStateOnBoot"
#endif

#ifndef G
#define G(n)                       String(F(n)).c_str()
#endif

namespace Pins {  static bool _master(false), _slave(false);
  class pin : public untyped {
    public:
      pin(short =-32768);
      virtual ~pin(){};

      inline pin&         name                  ( std::string s )    {if(_isActive()) {_changed|=(name()!=s);    at(G(ROUTE_PIN_NAME))=s;} return *this;};
      inline std::string  name                  ( void )             {return at(G(ROUTE_PIN_NAME)).c_str();};

      inline pin&         gpio                  ( short v )          {if(gpio()==-32768) {_changed|=(gpio()!=v); at(G(ROUTE_PIN_GPIO))=v;} return *this;};
      inline short        gpio                  ( void )             {return at(G(ROUTE_PIN_GPIO));};

      inline pin&         mode                  ( ushort m )         {if(_isActive()) {_changed|=(mode()!=m); pinMode( gpio(), (at(G(ROUTE_PIN_MODE))=m) );} return *this;};
      inline byte         mode                  ( void )             {return( at(G(ROUTE_PIN_MODE)) );};
      inline bool         inputMode             ( void )             {return( !outputMode() );};
      inline bool         outputMode            ( void )             {return( mode()==OUTPUT );};
      bool                isVirtual             ( void )             {return( gpio()<0 );};

      inline pin&         reverse               ( bool v )           {if(outputMode()) {_changed|=(reverse()!=v); at(G(ROUTE_PIN_REVERSE))=  v;} return *this;};
      inline bool         reverse               ( void )             {return at(G(ROUTE_PIN_REVERSE));};

      inline pin&         display               ( bool v=true )      {if(_isActive())  {_changed|=(hidden() ==v); at(G(ROUTE_PIN_HIDDEN)) = !v;} return *this;};
      inline bool         hidden                ( void )             {return at(G(ROUTE_PIN_HIDDEN));};

      inline pin&         blinking              ( bool v )           {if(outputMode()) {_changed|=(blinking()      !=v); at(G(ROUTE_PIN_BLINKING)) = v;}     return *this;};
      inline bool         blinking              ( void )             {return at(G(ROUTE_PIN_BLINKING));};
      inline pin&         blinkUpDelay          ( ulong v )          {if(outputMode()) {_changed|=(blinkUpDelay()  !=v); at(G(ROUTE_PIN_BLINKING_UP))  = v;} return *this;};
      inline ulong        blinkUpDelay          ( void )             {return at(G(ROUTE_PIN_BLINKING_UP));};
      inline pin&         blinkDownDelay        ( ulong v )          {if(outputMode()) {_changed|=(blinkDownDelay()!=v); at(G(ROUTE_PIN_BLINKING_DOWN))= v;} return *this;};
      inline ulong        blinkDownDelay        ( void )             {return at((ROUTE_PIN_BLINKING_DOWN));};

      inline pin&         timeout               ( ulong v )          {ulong t(timeout()); at(G(ROUTE_PIN_VALUE))=(_isActive() ?v :-1UL); _changed|=(t!=timeout()); return *this;};
      inline ulong        timeout               ( void )             {return at(G(ROUTE_PIN_VALUE));};
      inline pin&         unsetTimeout          ( void )             {return timeout(-1UL);};
      inline bool         isTimeout             ( void )             {if(_counter==-1UL || !_isNow(_counter)) return false; stopTimer(); return true;};
      void                startTimer            ( ulong =-1UL );
      inline void         stopTimer             ( void )             {_counter=-1UL;};
      inline pin&         isEnabled             ( bool v )           {_changed|=(isEnabled() != v); at(G(ROUTE_PIN_ENABLED)) = v; return *this;};
      inline bool         isEnabled             ( void )             {return at(G(ROUTE_PIN_ENABLED));};

      inline bool         isOn                  ( void )             {return at(G(ROUTE_PIN_STATE));};
      inline bool         isOff                 ( void )             {return !isOn();};
      inline pin&         set                   ( void )             {set(isOn()); return *this;};
      pin&                set                   ( bool, ulong =-1UL );

      inline void         mustRestore           ( bool v )           {if(_isActive())  {_changed|=(mustRestore() != v); at(G(ROUTE_RESTORE)) = v;}};
      inline bool         mustRestore           ( void )             {return at(G(ROUTE_RESTORE));};
      inline bool         changed               ( void )             {return _changed;};
      inline pin&         changed               ( bool force )       {_changed=force; return *this;};
      bool                saveToSD              ( String = "" );
      bool                restoreFromSD         ( String = "" );

      inline pin&         onTimeout             ( void(*f)() )       {_on_timeout=f;      return *this;};
      inline pin&         onBlinkUp             ( void(*f)() )       {_on_blinkup=f;      return *this;};
      inline pin&         onBlinkOut            ( void(*f)() )       {_on_blinkdown=f;    return *this;};
      inline pin&         onPinChange           ( void(*f)() )       {_on_state_change=f; return *this;};

    private:
      ulong               _counter, _nextBlink;     // delay counters;
      bool                _changed;
      String              _backupPrefix;
      void                (*_on_timeout)();
      void                (*_on_blinkup)();
      void                (*_on_blinkdown)();
      void                (*_on_state_change)();

      inline bool         _isActive             ( void )             {return (at(G(ROUTE_PIN_GPIO))>size_t(-32767));};
      bool                _restoreFromSD        ( String = "" );
      inline void         _serialSendState      ( bool reponseExpected=true )
                                                                     {if(Serial) Serial.print( (_master ?(reponseExpected ?G("S") :G("s")) :G("M")) + String(-gpio()-1,DEC) + G(":") + (isOn() ?G("1\n") :G("0\n")) );};

      inline static bool  _isNow                ( ulong v )          {ulong ms(millis()); return((v<ms) && (ms-v)<60000UL);};  //<-- Because of millis() rollover.

      friend class        pinsMap;
};

class pinsMap : public std::vector<pin>
  {
    public:
      pinsMap ( void ) : _backupPrefix("")      {_nullPin.gpio(-32767); _serialInputString.reserve(8);};
      ~pinsMap(){};

      inline pin&         push_back             ( short gpio )       {if( !exist(gpio) ) std::vector<pin>::push_back(pin(gpio)); return at(gpio);};
      inline pin&         operator()            ( short v )          {return at(v);};
      inline pin&         operator()            ( std::string v )    {return at(v);};
      inline bool         exist                 ( short v )          {for(auto &x: *this) if(x.gpio()==v) return true; return false;};
      inline bool         exist                 ( std::string v )    {for(auto &x: *this) if(x.name()==v) return true; return false;};
      inline pinsMap&     set                   ( void )             {for(auto &x: *this) at(x.gpio()).set();  return *this;};
      inline pinsMap&     set                   ( bool v )           {for(auto &x: *this) at(x.gpio()).set(v); return *this;};
      inline pin&         set                   ( ushort i, bool v ) {if(exist(i)) at(i).set(v); return at(i);};
      inline pin&         set                   ( ushort i )         {if(exist(i)) at(i).set();  return at(i);};
      pinsMap&            set                   ( untyped );
      inline pinsMap&     set                   ( std::vector<std::string> const &v )
                                                                     {for(auto &x :v) set( std::pair<std::string,untyped>{G(ROUTE_PIN_GPIO),untyped().deserializeJson(x)} ); return *this;};
      pinsMap&            mode                  ( ushort m )         {for(auto &x: *this) x.mode(m); return *this;};
      void                timers                ( void );
      inline pinsMap&     mustRestore           ( bool v )           {for(auto &x: *this) x.mustRestore( v ); return *this;};
      inline bool         mustRestore           ( void )             {for(auto &x: *this) if(x.mustRestore()) return true; return false;};
      pinsMap&            restoreFromSD         ( String = "" );
      inline pinsMap&     saveToSD              ( String prefix="" ) {if(prefix.length()) _backupPrefix=prefix;for(auto &x: *this) x.saveToSD(_backupPrefix); return *this;};
      inline pinsMap&     reset                 ( void )             {for(auto &x: *this) if(x.outputMode()) x.set(false); return *this;};
      pinsMap&            remove                ( ushort );
      inline pinsMap&     master                ( bool v )           {Pins::_master=v; return *this;};
      inline bool         master                ( void )             {return Pins::_master;};
      inline bool         slave                 ( void )             {return Pins::_slave;};
      void                serialEvent           ( void );
      inline void         serialSendReboot      ( void )             {if( master() ) if(Serial) Serial.print(F("S:??\n"));};
      inline void         serialSendMasterSearch( void )             {if(!master() ) if(Serial) Serial.print(F("M:??\n"));};
      inline pinsMap&     onPinChange           ( void(*f)() )       {for(auto &x :*this) x.onPinChange(f); return *this;};

    private:
      pin                 _nullPin;
      String              _backupPrefix, _serialInputString;

      inline pin&         at                    ( short v )          {for(auto &x: *this) if(x.gpio()==v) return x; return _nullPin;};
      inline pin&         at                    ( std::string v )    {for(auto &x: *this) if(x.name()==v) return x; return _nullPin;};
      inline void         _setAllPinsOnSlave    ( void )             {if( master() ) for(auto &x: *this) if(x.isVirtual()) x._serialSendState( false );};
      bool                _setSerialPin         ( void );
      bool                _serialPinsTreatment  ( void );
      inline static bool  _isInPins             ( std::string s )    {return(
            s==G(ROUTE_PIN_NAME)
        ||  s==G(ROUTE_PIN_GPIO)
        ||  s==G(ROUTE_PIN_MODE)
        ||  s==G(ROUTE_PIN_STATE)
        ||  s==G(ROUTE_PIN_REVERSE)
        ||  s==G(ROUTE_PIN_HIDDEN)
        ||  s==G(ROUTE_PIN_VALUE)
        ||  s==G(ROUTE_PIN_ENABLED)
        ||  s==G(ROUTE_PIN_BLINKING)
        ||  s==G(ROUTE_PIN_BLINKING_UP)
        ||  s==G(ROUTE_PIN_BLINKING_DOWN)
        ||  s==G(ROUTE_RESTORE)
      );};
  };
}
using namespace Pins;

#endif

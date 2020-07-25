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
/********************************* Serial pins Management ***********************************
  On the naster, the value of a serial gpio must be : (-1) * ( physicalSlaveGpioValue + 1 ).
  Examples:
    gpio 16 on slave is -17 on it's master,
    gpio  0 on slave is  -1 on it's master...
  Serial codes:
    From the master() to the slave():
      - "Sg:0" --> gpio(g) must be TRUE  on slave and new state return
      - "Sg:1" --> gpio(g) must be FALSE on slave and new state return
      - "sg:0" --> gpio(g) must be TRUE  on slave in optimistic mode
      - "sg:1" --> gpio(g) must be FALSE on slave in optimistic mode
      - "S:."  --> slave() must reboot.
    From the slave() to the master():
      - "M:?"  --> ask for a master,
      - "Mg:1" --> the new state of gpio(g) is now: TRUE 
      - "Mg:0" --> the new state of gpio(g) is now: FALSE 
      - "Mg:-" --> the timer of gpio(g) must be disabled.
********************************************************************************************/
#ifndef HEADER_FB360F1A9606491
#define HEADER_FB360F1A9606491

#include <Arduino.h>
#include <LittleFS.h>
#include <fstream>
#include "untyped.h"
#include "debug.h"

namespace Pins {
static bool _master(false), _slave(false);
// Json name attributes:
#define _NAME_                  "name"
#define _GPIO_                  "gpio"
#define _MODE_                  "mode"
#define _STATE_                 "state"
#define _REVERSE_               "reverse"
#define _HIDDEN_                "hidden"
#define _TIMEOUT_               "timeout"
#define _BLINKING_              "blinking"
#define _BLINKUP_               "blinkup"
#define _BLINKDOWN_             "blinkdown"
#define _RESTORE_               "restoreStateOnBoot"
#ifndef ulong
  #define ulong                 long unsigned int
#endif

template <typename T> std::string toString( T val ) {std::stringstream stream; stream << val; return stream.str();};

class pin : public untyped
  {
    public:
      pin(short =-32768);
      virtual ~pin(){};

      inline pin&         name                 ( std::string s )    {if(_isActive()) {_changed=(name()!=s);    at(_NAME_)=s;} return *this;};
      inline std::string  name                 ( void )             {return at(_NAME_).c_str();};

      inline pin&         gpio                 ( short v )          {if(gpio()==-32768) {_changed=(gpio()!=v); at(_GPIO_)=v;} return *this;};
      inline short        gpio                 ( void )             {return at(_GPIO_);};

      inline pin&         mode                 ( ushort m )         {if(_isActive()) {_changed=(mode()!=m); pinMode( gpio(), (at(_MODE_)=m) );} return *this;};
      inline ushort       mode                 ( void )             {return( at(_MODE_) );};
      inline bool         inputMode            ( void )             {return(_isActive() && mode()!=OUTPUT );};
      inline bool         outputMode           ( void )             {return(_isActive() && mode()==OUTPUT );};
      bool                isVirtual            ( void )             {return( gpio()<0 );};

      inline pin&         reverse              ( bool v )           {if(outputMode()) {_changed=(reverse()!=v); at(_REVERSE_)=  v;} return *this;};
      inline bool         reverse              ( void )             {return at(_REVERSE_);};

      inline pin&         display              ( bool v=true )      {if(_isActive())  {_changed=(hidden() ==v); at(_HIDDEN_) = !v;} return *this;};
      inline bool         hidden               ( void )             {return at(_HIDDEN_);};

      inline pin&         blinking             ( bool v )           {if(outputMode()) {_changed=(blinking()      !=v); at(_BLINKING_) = v;} return *this;};
      inline bool         blinking             ( void )             {return at(_BLINKING_);};
      inline pin&         blinkUpDelay         ( ulong v )          {if(outputMode()) {_changed=(blinkUpDelay()  !=v); at(_BLINKUP_)  = v;} return *this;};
      inline ulong        blinkUpDelay         ( void )             {return at(_BLINKUP_);};
      inline pin&         blinkDownDelay       ( ulong v )          {if(outputMode()) {_changed=(blinkDownDelay()!=v); at(_BLINKDOWN_)= v;} return *this;};
      inline ulong        blinkDownDelay       ( void )             {return at(_BLINKDOWN_);};

      pin&                timeout              ( ulong );
      inline ulong        timeout              ( void )             {return( at(_TIMEOUT_) );};
      inline pin&         unsetTimeout         ( void )             {_changed=(timeout()!=-1UL); at(_TIMEOUT_)=-1UL; return *this;};
      bool                isTimeout            ( void );
      void                startTimer           ( ulong =-1UL );
      inline void         stopTimer            ( void )             {_counter=-1UL;};

      inline bool         isOn                 ( void )             {if(outputMode()) at(_STATE_)=(digitalRead(gpio()) xor at(_REVERSE_).value<bool>()); return at(_STATE_);};
      inline bool         isOff                ( void )             {return !isOn();};
      pin&                set                  ( bool, ulong );
      inline pin&         set                  ( bool v )           {if(outputMode()) {_changed=(isOn()!=v && mustRestore()); set(v, timeout());} return *this;};

      inline void         mustRestore          ( bool v )           {if(_isActive())  {_changed=(mustRestore() != v); at(_RESTORE_) = v;}};
      inline bool         mustRestore          ( void )             {return at(_RESTORE_);};
      bool                saveToSD             ( void );
      bool                restoreFromSD        ( void );

    private:
      ulong               _counter, _nextBlink;     // delay counters;
      bool                _changed;

      inline bool         _isActive            ( void )             {return (at(_GPIO_)>size_t(-32767));};
      inline void         serialSendState      ( bool reponseExpected=true )
                                                                    {if(Serial) Serial.print( (_master ?(reponseExpected ?"S" :"s") :"M") + String(-gpio()-1,DEC) + ":" + (isOn() ?"1\n" :"0\n") );};
      inline static bool  _isNow               ( ulong v )          {ulong ms(millis()); return((v<ms) && (ms-v)<60000UL);};  //<-- Because of millis() rollover.
      friend class        pinsMap;

};

class pinsMap : public std::vector<pin>
  {
    public:
      pinsMap ( void );
      ~pinsMap(){};

      inline pin&         push_back            ( short gpio )       {if( !exist(gpio) ) std::vector<pin>::push_back(pin(gpio)); return at(gpio);};
      inline pin&         at                   ( short v )          {size_t i=indexOf(v);return ( i!=size_t(-1) ?operator[](i) : _nullPin);};
      inline pin&         at                   ( std::string v )    {size_t i=indexOf(v);return ( i!=size_t(-1) ?operator[](i) : _nullPin);};
      inline size_t       indexOf              ( short v )          {size_t i(0); for(auto x: *this) {if(x.gpio()==v) return i; i++;} return size_t(-1);};
      inline size_t       indexOf              ( std::string v )    {size_t i(0); for(auto x: *this) {if(x.name()==v) return i; i++;} return size_t(-1);};
      inline bool         exist                ( short v )          {return( indexOf(v)!=size_t(-1) );};
      inline bool         exist                ( std::string v )    {return( indexOf(v)!=size_t(-1) );};
      inline void         set                  ( size_t i, bool v ) {if(exist(i)) at(i).set(v);};
      void                timers               ( void );
      inline pinsMap&     mustRestore          ( bool v )           {for(auto x: *this) x.mustRestore( v ); return *this;};
      inline bool         mustRestore          ( void )             {for(auto x: *this) if(x.mustRestore()) return true; return false;};
      pinsMap&            restoreFromSD        ( void );
      inline pinsMap&     saveToSD             ( void )             {for(auto x: *this) x.saveToSD(); return *this;};
      inline pinsMap&     reset                ( void )             {for(auto x: *this) if(x.outputMode()) x.set(false); return *this;};
      pinsMap&            remove               ( size_t );
      pinsMap&            operator()           ( std::vector<std::string> v )
                                                                    {clear(); for(auto x:v) push_back(untyped().deserializeJson(x)); return *this;};
      inline pinsMap&     master               ( bool v )           {Pins::_master=v; return *this;};
      inline bool         master               ( void )             {return Pins::_master;};
      inline bool         slave                ( void )             {return Pins::_slave;};
      void                serialEvent          ( void );
      inline void         serialSendReboot     ( void )             {if( master() ) if(Serial) Serial.print("S:.\n");};
      inline void         serialSendMasterSearch( void )             {if(!master() ) if(Serial) Serial.print("M:?\n");};

    private:
      pin                 _nullPin;
      String              _serialInputString;

      inline void         _setAllPinsOnSlave   ( void )             {if( master() ) for(auto x: *this) if(x.isVirtual()) x.serialSendState( false );};
      bool                _serialPinsTreatment ( void );
  };
}

using namespace Pins;

#endif

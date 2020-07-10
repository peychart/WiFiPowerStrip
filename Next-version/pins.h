/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/untyped-cpp>

    Copyright (C) 2012  -  peychart

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
#ifndef HEADER_FB360F1A9606491
#define HEADER_FB360F1A9606491

#include <Arduino.h>
#include <LittleFS.h>
#include <fstream>
#include "untyped.h"
#include "debug.h"

namespace Pins {
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
#ifndef INFINY
  #define INFINY                60000UL
  inline bool isNow(ulong v)   {ulong ms(millis()); return((v<ms) && (ms-v)<INFINY);}  //Because of millis() rollover:
#endif
template <typename T> std::string toString( T val ) {std::stringstream stream; stream << val; return stream.str();};

class pin : private untyped
  {
    public:
      pin(short =-32768);

      virtual ~pin()        {};

      inline pin&           name           ( std::string s )      {if(_isActive()) {_changed=(name()!=s);    at(_NAME_)=s;} return *this;};
      inline std::string    name           ( void )               {return at(_NAME_).c_str();};

      inline pin&           gpio           ( short v )            {if(gpio()==-32768) {_changed=(gpio()!=v); at(_GPIO_)=v;} return *this;};
      inline short          gpio           ( void )               {return at(_GPIO_);};

      inline pin&           mode           ( char m )             {if(_isActive()) {_changed=(mode()!=m); pinMode( gpio(), (at(_MODE_)=m) );} return *this;};
      inline char           mode           ( void )               {return at(_MODE_);};
      inline bool           inputMode      ( void )               {return(_isActive() && mode()!=OUTPUT );};
      inline bool           outputMode     ( void )               {return(_isActive() && mode()==OUTPUT );};
      bool                  isVirtual      ( void )               {return( gpio()<0 );};

      inline pin&           reverse        ( bool v )             {if(outputMode()) {_changed=(reverse()!=v); at(_REVERSE_)= v;} return *this;};
      inline bool           reverse        ( void )               {return at(_REVERSE_);};

      inline pin&           display        ( bool v=true )        {if(_isActive())  {_changed=(hidden() !=v); at(_HIDDEN_) = v;} return *this;};
      inline bool           hidden         ( void )               {return at(_HIDDEN_);};

      inline pin&           blinking       ( bool v )             {if(outputMode()) {_changed=(blinking()      !=v); at(_BLINKING_)= v;} return *this;};
      inline bool           blinking       ( void )               {return at(_BLINKING_);};
      inline pin&           blinkUpDelay   ( ulong v )            {if(outputMode()) {_changed=(blinkUpDelay()  !=v); at(_BLINKUP_) = v;} return *this;};
      inline ulong          blinkUpDelay   ( void )               {return at(_BLINKUP_);};
      inline pin&           blinkDownDelay ( ulong v )            {if(outputMode()) {_changed=(blinkDownDelay()!=v); at(_BLINKDOWN_)=v;} return *this;};
      inline ulong          blinkDownDelay ( void )               {return at(_BLINKDOWN_);};

      pin&                  setTimeout     ( ulong );
      inline ulong          timeout        ( void )               {return( at(_TIMEOUT_) );};
      inline pin&           unsetTimeout   ( void )               {_changed=(timeout()!=-1UL); at(_TIMEOUT_)=-1UL; return *this;};
      bool                  isTimeout      ( void );
      void                  startTimer     ( ulong =-1UL );
      inline void           stopTimer      ( void )               {_counter=-1UL;};

      inline bool           isOn           ( void )               {if(outputMode()) at(_STATE_)=(digitalRead(at(_GPIO_)) xor at(_REVERSE_).value<bool>()); return at(_STATE_);};
      inline bool           isOff          ( void )               {return !isOn();};
      pin&                  set            ( bool, ulong );
      inline pin&           set            ( bool v )             {_changed=(isOn()!=v && mustRestore()); if(outputMode()) {set(v, at(_TIMEOUT_));} return *this;};

      inline void           mustRestore    ( bool v )             {_changed=(mustRestore() != v); at(_RESTORE_) = v;};
      inline bool           mustRestore    ( void )               {return at(_RESTORE_);};
      bool                  saveToSD       ( void );
      bool                  restoreFromSD  ( void );

    private:
      ulong                 _counter, _nextBlink;     // delay counters;
      bool                  _changed;

      inline bool           _isActive      ( void )               {return (at(_GPIO_)>-32767);};

      friend class          pinMap;
  };

class pinMap : public std::vector<pin*>
  {
    public:
      pinMap();
      ~pinMap() {for(auto x: list()) delete x;};

      inline ushort         inputCount     ( void )               {ushort n(0); for(auto x : list()) if(x->inputMode() ) n++; return n;};
      inline ushort         outputCount    ( void )               {ushort n(0); for(auto x : list()) if(x->outputMode()) n++; return n;};
      inline ushort         virtualCount   ( void )               {ushort n(0); for(auto x : list()) if(x->gpio()<0L)    n++; return n;};

      inline pin&           push_back      ( short gpio )         {if( !exist(gpio) ) std::vector<pin*>::push_back(new pin(gpio)); return at(gpio);};
      inline pin&           at             ( short v )            {size_t i=indexOf(v);return ( i!=size_t(-1) ?*operator[](i) : nullPin);};
      inline pin&           at             ( std::string v )      {size_t i=indexOf(v);return ( i!=size_t(-1) ?*operator[](i) : nullPin);};
      inline size_t         indexOf        ( short v )            {size_t i(0); for(auto x: list()) {if(x->gpio()==v) return i; i++;} return size_t(-1);};
      inline size_t         indexOf        ( std::string v )      {size_t i(0); for(auto x: list()) {if(x->name()==v) return i; i++;} return size_t(-1);};
      inline bool           exist          ( short v )            {return( indexOf(v)!=size_t(-1) );};
      inline bool           exist          ( std::string v )      {return( indexOf(v)!=size_t(-1) );};
      void                  timers         ( void );
      inline pinMap&        mustRestore    ( bool v )             {for(auto x: list()) x->mustRestore( v ); return *this;};
      inline bool           mustRestore    ( void )               {for(auto x: list()) if(x->mustRestore()) return true; return false;};
      pinMap&               restoreFromSD  ( void );
      inline pinMap&        saveToSD       ( void )               {for(auto x: list()) x->saveToSD(); return *this;};
      inline pinMap&        reset          ( void )               {for(auto x: list()) if(x->outputMode()) x->set(false); return *this;};
      pinMap&               remove         ( ushort );

    private:
      pin                   nullPin;
      inline pinMap&        list           ( void )               {return(*this);};
  };
}

using namespace Pins;

#endif

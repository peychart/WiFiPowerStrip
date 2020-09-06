/* ESP8266-Switches-Manager C++ (Version 0.1 - 2020/07)
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
#include "switches.h"

namespace Switches
{
  void switches::inputEvent( volatile bool& intr, volatile ulong& rebound_completed ) {
    if( intr || _in_progress ){
      if( !_interruptTraitement )
        intr=_in_progress=false;
      else if(_isNow( rebound_completed ) ){    //switch is now activated...
        (this->*_interruptTraitement)(); intr = _pushCount;
  } } }

  ushort switches::_getInputs(uint16_t reg){
    ushort i(0), n(0);
    for(auto x : *this){
      if( (reg & (1<<x.gpio())) == (x.mode()==INPUT) )
        n += (1<<i);
      i++;
    }return n;
  }

  void switches::_treatment_1() {                //<-- Switches.size() <= _outPins.size():
    ushort n(_getInputs(GPI) );
    if( !_pushCount ){
      _in_progress=(_pushCount=n);
      _next_timerDisabler = millis() + HOLD_TO_DISABLE_TIMER * 1000UL;
      DEBUG_print(F("\nIO init: ")); for(ushort i(size()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0); DEBUG_print(F("\n"));
    }else if( n && n!=_pushCount ){              // error
      _in_progress=(_pushCount=0);
      DEBUG_print(F("\nIO ERROR.\n")); for(ushort i(size()); i; i--) DEBUG_print( 1<<(i-1) );
    }else if( !n ){
      if( --_pushCount < _outPins.size() ){
        if(_isNow(_next_timerDisabler ) ){
          if(_outPins[_pushCount ].isOff() )
            _outPins[_pushCount ].set(_outPins[_pushCount ].isOff() );   //<-- set output
          _outPins[_pushCount ].stopTimer();
          DEBUG_print(F("Timer removed on ")); DEBUG_print(String(operator()(n).gpio(),DEC)); DEBUG_print(F("(")); DEBUG_print(_outPins[n].name().c_str()); DEBUG_print(F(")")); DEBUG_print(F("\n"));
        }else _outPins[_pushCount ].set(_outPins[_pushCount ].isOff() );  //<-- switch output
        _in_progress=(_pushCount=0 );
  } } }

  void switches::_treatment_2() {                //<-- single-switch multi-outputs:
    static bool lock(false);
    ushort n(_getInputs(GPI) ); _in_progress = true;
    if ( n && !lock ){ // switch activation.
      _next_timerDisabler = millis() + HOLD_TO_DISABLE_TIMER * 1000UL;
      _cmd_completed      = millis() + CMD_COMPLETED_TIMER   * 1000UL;
      _pushCount++; lock=true;
      DEBUG_print(F("\nIO init: ")); for(ushort i(size()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0); DEBUG_print(F("\n"));
    }else if(_isNow(_cmd_completed ) || _isNow(_next_timerDisabler ) ){
      if( --_pushCount < _outPins.size() ){
        _outPins[_pushCount ].set( _outPins[_pushCount ].isOff() );    //<-- (un)set output
        if( n && _isNow(_next_timerDisabler ) ){ //<-- Unset output timer
          _outPins(_pushCount ).stopTimer();
          DEBUG_print(F("Timer removed on ")); DEBUG_print(String(operator()(n).gpio(),DEC)); DEBUG_print(F("(")); DEBUG_print(_outPins[n].name().c_str()); DEBUG_print(F(")")); DEBUG_print(F("\n"));
        }lock=_in_progress=(_pushCount=0);
    } }
    else if( !n )
      lock=(_pushCount=0);
  }

}

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
  void switches::event( volatile bool& intr, volatile ulong& rebound_completed ) {
    if( intr || _in_progress ){
      if( !_interruptTraitement )
        intr=_in_progress=false;
      else if( _isNow( rebound_completed ) ){  //switch is now activated...
        (this->*_interruptTraitement)(); intr = _pushCount;
  } } }

  ushort switches::_getInputs(uint16_t reg){
    ushort n=0;
    for(ushort i(0); i<size(); i++)
      if((reg&(1<<at(i).gpio()))==0)
        n+=(1<<i);
    return n;
  }

  void switches::_treatment_1() {              //<-- Switches.size() <= _outPins.size():
    ushort n( _getInputs(GPI) );
    if( !_pushCount ){
      _in_progress=(_pushCount=n);
      _next_timerDisabler = millis() + HOLD_TO_DISABLE_TIMER * 1000UL;
      DEBUG_print("\nIO init: ");   for(ushort i(size()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
    }else if( n && n!=_pushCount ){                 // error
      _in_progress=(_pushCount=0);
      DEBUG_print("\nIO ERROR.\n"); for(ushort i(size()); i; i--) DEBUG_print( 1<<(i-1) );
    }else if( !n || _isNow( _next_timerDisabler ) ){
      _pushCount--;
      _setOutput( _pushCount );                 // switch output
      if( _isNow( _next_timerDisabler ) )
        _unsetTimeout( _pushCount );            // Unset output timeout
      _in_progress=(_pushCount=0);
  } }

  void switches::_treatment_2() {               //<-- only one switch for several outpins:
    static bool lock(false);
    ushort n( _getInputs(GPI) ); _in_progress = true;
    if ( n && !lock ){ //the switch has been switched.
      _next_timerDisabler = millis() + HOLD_TO_DISABLE_TIMER * 1000UL;
      _cmd_completed      = millis() + CMD_COMPLETED_TIMER   * 1000UL;
      _pushCount++; lock=true;
      DEBUG_print("\nIO init: "); for(ushort i(size()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
    }else if( _isNow( _cmd_completed ) || _isNow( _next_timerDisabler ) ){
      _pushCount--;
      _setOutput( _pushCount );                 // (un)set output
      if( n && _isNow( _next_timerDisabler ) )  // Unset output timer
        _unsetTimeout( _pushCount );
      lock=_in_progress=(_pushCount=0);
    }else if( !n ){
      lock=(_pushCount=0);
  } }

  void switches::_setOutput( ushort n ) {
    DEBUG_print("IO : "); for(size_t i(size()); i; i--) DEBUG_print(1<<(i-1)); DEBUG_print("\n");
    DEBUG_print("GPI: "); for(size_t i(size()); i; i--) DEBUG_print((n+1)&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
    if( n<size() ){
      if( _isNow( _next_timerDisabler ) ){      // --> the gpio timer hax been disabled...
        (*this)[n].set( (*this)[n].isOn(), -1UL );
        DEBUG_print( ("Timer removed on " + String((*this)[n].gpio(), DEC) + "(" + (*this)[n].name().c_str() + ")\n").c_str() );
      }else (*this)[n].set( !(*this)[n].isOn() );
      if( (*this)[n].isOff() ) (*this)[n].unsetTimeout();
  } }

  void switches::_unsetTimeout( ushort n ) {
    if( n<size() ){
      (*this)[n].unsetTimeout();
      DEBUG_print( "Timer removed on " + String(at(n).gpio(), DEC) + "(" + (*this)[n].name().c_str() + ")\n" );
  } }
}

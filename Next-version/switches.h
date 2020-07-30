/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/switches-cpp>

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
#ifndef HEADER_EC899EF02154031
#define HEADER_EC899EF02154031

#include <Arduino.h>
#include "setting.h"
#include "pins.h"
#include "debug.h"

namespace Switches {

 class switches : public pinsMap
 {
  public:
    switches( pinsMap &p ) : _count(0), _in_progress(false), _outPins(p), _next_timerDisabler(0UL), _cmd_completed(0UL), _interruptTraitement(0) {};

    inline switches&    init            ( void(*f)(), char mode )           {_attachAll(f, mode ); _setTraitement();   return *this;};
    inline switches&    init            ( void(*f)(), char m, size_t g )    {_attachOne(f, m, g ); _setTraitement();   return *this;};
    inline switches&    reset           ( void )                            {                      _unsetTraitement(); return *this;};
    void                event           ( volatile bool&, volatile ulong& );

  private:
    ushort              _count;
    bool                _in_progress;
    pinsMap&            _outPins;
    unsigned long       _next_timerDisabler, _cmd_completed;
    void                (switches::*_interruptTraitement)();

    inline void         _unsetTraitement( void )                            {_interruptTraitement=0;};
    inline void         _setTraitement  ( void )                            {_interruptTraitement=( (size()<=_outPins.size()) ?&switches::_treatment_1 :&switches::_treatment_2 );};
    inline void         _attachOne      ( void(*f)(), char m, size_t g )    {if (indexOf(g)!=size_t(-1)) attachInterrupt( digitalPinToInterrupt(g), f, m );};
    inline void         _attachAll      ( void(*f)(), char m )              {for( auto &x: *this ) _attachOne( f, m, x.gpio() );};
    ushort              _getInputs      ( uint16_t reg );
    void                _treatment_1    ( void );
    void                _treatment_2    ( void );
    void                _setOutput      ( ushort );
    void                _unsetTimeout   ( ushort );
    inline static bool  _isNow          ( ulong v )                         {ulong ms(millis()); return((v<ms) && (ms-v)<60000UL);};  //<-- Because of millis() rollover.
 };
}
using namespace Switches;

#endif

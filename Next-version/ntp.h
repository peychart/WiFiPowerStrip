/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/untyped-cpp>

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
#ifndef HEADER_EC332E292901801
#define HEADER_EC332E292901801

#include <Arduino.h>
#include <WiFiClient.h>
#include <NtpClientLib.h>
#include <LittleFS.h>
#include "untyped.h"
#include "setting.h"
#include "debug.h"

namespace _NTP {
// Json name attributes:
#define _NTP_DISABLED_     "disabled"
#define _NTP_SOURCE_       "source"
#define _NTP_ZONE_         "zone"
#define _NTP_DAYLIGHT_     "dayLight"
#define _NTP_INTERVAL_     "interval"

 class ntp : public untyped
 {
  public:
    ntp ( void );

    virtual ~ntp()     {saveToSD();};

    inline ntp&         disabled       ( bool v )         {_changed|=(at(_NTP_DISABLED_)!= v); at(_NTP_DISABLED_) = v; this->begin(); return *this;};
    inline ntp&         source         ( std::string v )  {_changed|=(at(_NTP_SOURCE_)  != v); at(_NTP_SOURCE_)   = v; return *this;};
    inline ntp&         zone           ( short v )        {_changed|=(at(_NTP_ZONE_)    != v); at(_NTP_ZONE_)     = v; return *this;};
    inline ntp&         dayLight       ( bool v )         {_changed|=(at(_NTP_DAYLIGHT_)!= v); at(_NTP_DAYLIGHT_) = v; return *this;};
    inline ntp&         interval       ( ulong v )        {_changed|=(at(_NTP_INTERVAL_)!= v); at(_NTP_INTERVAL_) = v; return *this;};
    inline bool         disabled       ( void )           {return !source().empty() && at(_NTP_DISABLED_);};
    inline std::string  source         ( void )           {return at(_NTP_SOURCE_  ).c_str();};
    inline short        zone           ( void )           {return at(_NTP_ZONE_    );};
    inline bool         dayLight       ( void )           {return at(_NTP_DAYLIGHT_);};
    inline ulong        interval       ( void )           {return at(_NTP_INTERVAL_);};

    void                begin          ( void );
    inline bool         isSynchronized ( ulong t=now() )  {return( t>-1UL/10UL );};
    inline void         getTime        ( void )           {if( !source().empty() && !isSynchronized() ) {DEBUG_print("Retry NTP synchro...\n"); NTP.getTime();};};

    bool                saveToSD       ( void );
    bool                restoreFromSD  ( void );

  private:
    bool                _changed;
 };
}
using namespace _NTP;

#endif

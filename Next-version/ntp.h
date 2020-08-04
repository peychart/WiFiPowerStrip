/* ESP8266-NTP-Manager C++ (Version 0.1 - 2020/07)
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
#ifndef HEADER_EC332E292901801
#define HEADER_EC332E292901801

#include <Arduino.h>
#include <WiFiClient.h>
#include <NtpClientLib.h>
#include <LittleFS.h>
#include "untyped.h"
#include "setting.h"
#include "debug.h"

// Json name attributes:
#ifdef ROUTE_NTP_SOURCE
  #define ROUTE_NTP_SOURCE    "ntpSource"
  #define ROUTE_NTP_ZONE      "ntpZone"
  #define ROUTE_NTP_DAYLIGHT  "ntpDayLight"
#endif
#define ROUTE_NTP_INTERVAL    "ntpInterval"
#define ROUTE_NTP_DISABLED    "ntpDisabled"

namespace _NTP {
 class ntp : public untyped {
  public:
    ntp ( void );

    virtual ~ntp()     {saveToSD();};

    inline ntp&         disabled       ( bool v )         {_changed|=(at(ROUTE_NTP_DISABLED)!= v); at(ROUTE_NTP_DISABLED) = v; this->begin(); return *this;};
    inline ntp&         source         ( std::string v )  {_changed|=(at(ROUTE_NTP_SOURCE)  != v); at(ROUTE_NTP_SOURCE)   = v; return *this;};
    inline ntp&         zone           ( short v )        {_changed|=(at(ROUTE_NTP_ZONE)    != v); at(ROUTE_NTP_ZONE)     = v; return *this;};
    inline ntp&         dayLight       ( bool v )         {_changed|=(at(ROUTE_NTP_DAYLIGHT)!= v); at(ROUTE_NTP_DAYLIGHT) = v; return *this;};
    inline ntp&         interval       ( ulong v )        {_changed|=(at(ROUTE_NTP_INTERVAL)!= v); at(ROUTE_NTP_INTERVAL) = v; return *this;};
    inline bool         disabled       ( void )           {return !source().empty() && at(ROUTE_NTP_DISABLED);};
    inline std::string  source         ( void )           {return at(ROUTE_NTP_SOURCE  ).c_str();};
    inline short        zone           ( void )           {return at(ROUTE_NTP_ZONE    );};
    inline bool         dayLight       ( void )           {return at(ROUTE_NTP_DAYLIGHT);};
    inline ulong        interval       ( void )           {return at(ROUTE_NTP_INTERVAL);};

    void                begin          ( void );
    inline bool         isSynchronized ( ulong t=now() )  {return( t>-1UL/10UL );};
    inline void         getTime        ( void )           {if( !source().empty() && !isSynchronized() ) {DEBUG_print("Retry NTP synchro...\n"); NTP.getTime();};};

    ntp&                set            ( untyped );
    bool                saveToSD       ( void );
    bool                restoreFromSD  ( void );

  private:
    bool                _changed;
    inline static bool  _isInMqtt      ( std::string s )      {return(
          s==ROUTE_NTP_SOURCE
      ||  s==ROUTE_NTP_ZONE
      ||  s==ROUTE_NTP_DAYLIGHT
      ||  s==ROUTE_NTP_INTERVAL
      ||  s==ROUTE_NTP_DISABLED
    );};
 };
}
using namespace _NTP;

#endif

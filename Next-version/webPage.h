/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/untyped-cpp>

    Copyright (C) 2017  -  peychart

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
#ifndef HEADER_FB360F2C9804236
#define HEADER_FB360F2C9804236

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"
#include "pins.h"
#include "mqtt.h"
#include "ntp.h"

#ifndef REFRESH_PERIOD
#define REFRESH_PERIOD 30
#endif

extern ESP8266WebServer ESPWebServer;
extern WiFiManager      myWiFi;
extern pinMap           myPins;
extern mqtt             myMqtt;
extern ntp              myNTP;

void                    setupWebServer( void );
void                    handleRoot    ( void );
void                    setConfig     ( void );
char const*             getConfig     ( void );
char const*             getStatus     ( void );
void                    reboot        ( void );
inline std::string      Upper         ( std::string s ) {std::for_each(s.begin(), s.end(), [](char & c){c = ::toupper(c);}); return s;};
inline std::string      Lower         ( std::string s ) {std::for_each(s.begin(), s.end(), [](char & c){c = ::tolower(c);}); return s;};

#endif

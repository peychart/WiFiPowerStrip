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
#ifndef HEADER_FB360F2C9804236
#define HEADER_FB360F2C9804236

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"
#include "pins.h"
#include "mqtt.h"
#include "ntp.h"
#include "debug.h"

#include "setting.h"

//HTML SCHEMA:
#ifndef REFRESH_PERIOD
#define REFRESH_PERIOD 30
#endif

extern ESP8266WebServer ESPWebServer;
extern WiFiManager      myWiFi;
extern pinsMap          myPins;
#ifdef  DEFAULT_MQTT_BROKER
  extern mqtt           myMqtt;
#endif
#ifdef DEFAULT_NTPSOURCE
  extern ntp              myNTP;
#endif

void                    setupWebServer ( void );
void                    handleRoot     ( void );
bool                    htmlSend       ( short );
String                  getMqttSchema  ( String="" );
void                    setConfig      ( void );
String                  sendConfigToJS ( void );
String                  sendStatesToJS ( void );
inline String           get            ( std::string n, untyped v ) {return untyped(std::pair<std::string,untyped>{n,v}).serializeJson().c_str();};
void                    reboot         ( void );
inline std::string      Upper          ( std::string s )            {std::for_each(s.begin(), s.end(), [](char & c){c = ::toupper(c);}); return s;};
inline std::string      Lower          ( std::string s )            {std::for_each(s.begin(), s.end(), [](char & c){c = ::tolower(c);}); return s;};
inline std::string      ltrim          ( std::string s, const std::string& chars = "\t\n\v\f\r " )
                                                                    {s.erase(0, s.find_first_not_of(chars)); return s;};
inline std::string      rtrim          ( std::string s, const std::string& chars = "\t\n\v\f\r " )
                                                                    {s.erase(s.find_last_not_of(chars) + 1); return s;};
inline std::string      trim           ( std::string s, const std::string& chars = "\t\n\v\f\r " )
                                                                    {return ltrim(rtrim(s, chars), chars);};

#endif

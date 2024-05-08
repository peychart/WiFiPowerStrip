/* ESP8266-WEB-Manager C++ (Version 0.1 - 2020/07)
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
#ifndef HEADER_FB360F2C9804236
#define HEADER_FB360F2C9804236

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"
#include "pins.h"
#include "mqtt.h"
#ifdef DEFAULT_NTPSOURCE
  #include "ntp.h"
#endif

#include "setting.h"
#include "debug.h"

extern ESP8266WebServer    ESPWebServer;
extern WiFiManager         myWiFi;
extern pinsMap             myPins;
#ifndef ACCESS_CONTROL_ALLOW_ORIGIN
  #define ACCESS_CONTROL_ALLOW_ORIGIN "*.home.lan"
#endif
#ifndef  DEFAULTHOSTNAME
  #define DEFAULTHOSTNAME "ESP8266"
#endif
#ifndef  DEFAULTWIFIPASS
  #define DEFAULTWIFIPASS "defaultPassword"
#endif
#ifdef  DEFAULT_MQTT_BROKER
  extern mqtt              myMqtt;
#endif
#ifdef DEFAULT_NTPSOURCE
  extern ntp               myNTP;
#endif
#ifndef G
  #define G(n)             String(F(n)).c_str()
#endif

void                       setupWebServer       ( void );
void                       handleRoot           ( bool = true );
#ifdef EXTERN_WEBUI
String                     HTML_redirHeader     ( void );
#else
const __FlashStringHelper* HTML_Header          ( void );
const __FlashStringHelper* HTML_MainForm        ( void );
const __FlashStringHelper* HTML_AboutPopup      ( void );
const __FlashStringHelper* HTML_ConfPopup       ( void );
const __FlashStringHelper* HTML_JRefresh        ( void );
const __FlashStringHelper* HTML_JSubmits        ( void );
const __FlashStringHelper* HTML_JMainDisplay    ( void );
const __FlashStringHelper* HTML_JSSIDDisplay    ( void );
const __FlashStringHelper* HTML_JMQTTDisplay    ( void );
#endif
void                       sendDeviceStatusToJS ( void );
void                       configDeviceFromJS   ( void );

extern void                reboot               ( void );
#endif

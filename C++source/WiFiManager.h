/* ESP8266-WiFi-Manager C++ (Version 0.1 - 2020/07)
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
/*
  Connect in AP mode or STA mode if ssid(s) declared.
  Retry 3 times every 30s on disconnected... either -> AP mode is set for 5mn (then, retry).
  Allows callback functions on events...

  Use examples:
    WiFiDaemon("ESP8266").push_back("mySSID-1", "myPWD-1").connect();  // -> STA mode for ESP8266.local...
    ...
    WiFiDaemon.disconnect(n);      // if n=0L -> do not try to reconnect, other value = reconnect in n milli-seconds, no value -> n=30s...
    ...
    WiFiDaemon.aPModeDenied();     // no AP mode when ssid(s) is(are) declared...
    ...
    WiFiDaemon().clear();          // disconnect, no more ssid, do not try to reconnect...
    WiFiDaemon().connect();        // -> AP mode with default hostname="ESP8266-XXXXXXX.local" and ip=192.168.4.1
*/
#ifndef HEADER_FB324C728556321
#define HEADER_FB324C728556321

#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include "untyped.h"

#include "setting.h"
#include "debug.h"

#define AP_IPADDR      IPAddress(192,168,4,1)
#define AP_GATEWAY     IPAddress(192,168,4,254)
#define AP_SUBNET      IPAddress(255,255,255,0)

namespace WiFiManagement {
#ifndef DEFAULTWIFIPASS
  #define DEFAULTWIFIPASS    "defaultPassword"
#endif
#define NAME_MAX_LEN          16
#ifndef G
  #define G(n)                String(F(n)).c_str()
#endif

// Json name attributes:
#ifndef ROUTE_VERSION
  #define ROUTE_VERSION      "version"
  #define ROUTE_HOSTNAME     "hostname"
  #define ROUTE_PIN_VALUE    "timeout"
  #define ROUTE_WIFI_SSID    "ssid"
  #define ROUTE_WIFI_PWD     "pwd"
#endif

  class WiFiManager : public untyped {
    public:
      WiFiManager();

      virtual ~WiFiManager()   {saveToSD();};

      inline std::string        version          ( void )                   {return at(G(ROUTE_VERSION)).c_str();};
      inline WiFiManager&       version          ( std::string s )          {_changed|=(version()!=s); at(G(ROUTE_VERSION))=s; return *this;};
      WiFiManager&              hostname         ( std::string );
      inline std::string        hostname         ( void )                   {return at(G(ROUTE_HOSTNAME)).c_str();};
      inline bool               enabled          ( void )                   {return _enabled;};
      inline bool               disabled         ( void )                   {return !enabled();};
      inline size_t             ssidCount        ( void )                   {return at(G(ROUTE_WIFI_SSID)).vectorSize();};
      inline size_t             indexOf          ( std::string s )          {for(size_t i(0); i<ssidCount(); i++) if(ssid(i)==s) return i; return size_t(-1);};
      WiFiManager&              push_back        ( std::string, std::string );
      inline ulong              reconnectionTime ( void )                   {return at(G(ROUTE_PIN_VALUE));};
      inline WiFiManager&       ssid             ( size_t i, std::string s ){if (i<ssidCount()) {_changed|=(ssid(i)!=s); at(G(ROUTE_WIFI_SSID)).vector().at(i) = s;}; return *this;};
      inline std::string        ssid             ( size_t i )               {if (i<ssidCount()) return at(G(ROUTE_WIFI_SSID)).vector().at(i).c_str();   return "";};
      inline WiFiManager&       reconnectionTime ( ulong v )                {_changed|=(reconnectionTime()!=v); at(G(ROUTE_PIN_VALUE)) = v; return *this;};
      inline WiFiManager&       password         ( size_t i, std::string p ){if (i<ssidCount()) push_back(ssid(i), p);  return *this;};
      inline std::string        password         ( size_t i )               {if (i<ssidCount()) return at(G(ROUTE_WIFI_PWD)).at(i).c_str(); return "";};
      WiFiManager&              erase            ( size_t );
      inline WiFiManager&       clear            ( void )                   {if(ssidCount()) _changed=true; at(G(ROUTE_WIFI_SSID)).clear(); at(G(ROUTE_WIFI_PWD)).clear(); disconnect(0L); return *this;};

      bool                      connect          ( void );
      inline bool               apConnected      ( void )                   {return _ap_connected;};
      inline bool               staConnected     ( void )                   {return (WiFi.status()==WL_CONNECTED);};
      inline bool               connected        ( void )                   {return (apConnected() || staConnected());};
      inline bool               disconnected     ( void )                   {return !connected();};
      WiFiManager&              disconnect       ( ulong );
      inline WiFiManager&       disconnect       ( void )                   {return disconnect(at(G(ROUTE_PIN_VALUE)));};
      void                      loop             ( void );
      inline WiFiManager&       aPModeDenied     ( void )                   {_apMode_enabled=0;   return *this;};
      inline WiFiManager&       apModeAllowed    ( size_t n=-1 )            {_apMode_enabled=n;   return *this;};
      inline bool               changed          ( void )                   {return _changed;};
      inline WiFiManager&       changed          ( bool force )             {_changed=force; return *this;};

      inline WiFiManager&       setOutputPower   ( float power )            {WiFi.setOutputPower(power); return *this;};

      inline WiFiManager&       onConnect        ( void(*f)() )             {_on_connect=f;       return *this;};
      inline WiFiManager&       onApConnect      ( void(*f)() )             {_on_apConnect=f;     return *this;};
      inline WiFiManager&       onStaConnect     ( void(*f)() )             {_on_staConnect=f;    return *this;};
      inline WiFiManager&       onDisconnect     ( void(*f)() )             {_on_disconnect=f;    return *this;};
      inline WiFiManager&       onApDisconnect   ( void(*f)() )             {_on_apDisconnect=f;  return *this;};
      inline WiFiManager&       onStaDisconnect  ( void(*f)() )             {_on_staDisconnect=f; return *this;};
      inline WiFiManager&       ifConnected      ( void(*f)() )             {_if_connected=f;     return *this;};
      inline WiFiManager&       ifApConnected    ( void(*f)() )             {_if_apConnected=f;   return *this;};
      inline WiFiManager&       ifStaConnected   ( void(*f)() )             {_if_staConnected=f;  return *this;};

      WiFiManager&              set              ( untyped );
      bool                      saveToSD         ( void );
      bool                      restoreFromSD    ( void );

    private:
      bool                      _enabled;
      bool                      _ap_connected;
      size_t                    _apMode_enabled;
      void                      (*_on_connect)();
      void                      (*_on_apConnect)();
      void                      (*_on_staConnect)();
      void                      (*_on_disconnect)();
      void                      (*_on_apDisconnect)();
      void                      (*_on_staDisconnect)();
      void                      (*_if_connected)();
      void                      (*_if_apConnected)();
      void                      (*_if_staConnected)();
      bool                      _changed;
      ulong                     _next_connect;
      byte                      _trial_counter, _apTimeout_counter;
      const byte                _trialNbr=3, _apTimeout=10;

      bool                      _apConnect();

      inline static bool        _isNow           ( ulong v )                {ulong ms(millis()); return((v<ms) && (ms-v)<60000UL);};  //<-- Because of millis() rollover.
      inline static bool        _isInWiFiManager ( std::string s )          {return(
            s==G(ROUTE_VERSION)
        ||  s==G(ROUTE_HOSTNAME)
        ||  s==G(ROUTE_PIN_VALUE)
        ||  s==G(ROUTE_WIFI_SSID)
        ||  s==G(ROUTE_WIFI_PWD)
      );};
  };
}
using namespace WiFiManagement;

#endif

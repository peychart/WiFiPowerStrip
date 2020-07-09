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
#include "debug.h"

#define AP_IPADDR      IPAddress(192,168,4,1)
#define AP_GATEWAY     IPAddress(192,168,4,254)
#define AP_SUBNET      IPAddress(255,255,255,0)

extern untyped myConf;

namespace WiFiManagement {
#ifndef DEFAULTWIFIPASS
  #define DEFAULTWIFIPASS    "defaultPassword"
#endif
#define NAME_MAX_LEN          16
#define SSID_VECTOR_MAX_SIZE  3
#define MEMORYLEAKS           5000UL
// Json name attributes:
#define _VERSION_            "version"
#define _HOSTNAME_           "hostname"
#define _TIMEOUT_            "timeout"
#define _SSID_               "ssid"
#undef  _PWD_
#define _PWD_                "pwd"
#ifndef ulong
  #define ulong  long unsigned int
#endif
//#ifndef ushort
//  #define ushort   unsigned short
//#endif
#ifndef INFINY
  #define INFINY               60000UL
  inline bool isNow(ulong v)  {ulong ms(millis()); return((v<ms) && (ms-v)<INFINY);}  //Because of millis() rollover:
#endif

  class WiFiManager : private untyped
  {
    public:
      WiFiManager();

      virtual ~WiFiManager()   {saveToSD();};

      inline std::string        version          ( void )                   {return at(_VERSION_);};
      inline WiFiManager&       version          ( std::string s )          {_changed=(version()!=s); at(_VERSION_)==s; return *this;};
      WiFiManager&              hostname         ( std::string );
      inline std::string        hostname         ( void )                   {return at(_HOSTNAME_).c_str();};
      inline bool               enabled          ( void )                   {return _enabled;};
      inline bool               disabled         ( void )                   {return !enabled();};
      inline size_t             ssidMaxCount     ( void )                   {return SSID_VECTOR_MAX_SIZE;};
      inline size_t             ssidCount        ( void )                   {return at(_SSID_).vectorSize();};
      size_t                    indexOf          ( std::string );
      WiFiManager&              push_back        ( std::string, std::string );
      inline ulong              reconnectionTime ( void )                   {return at(_TIMEOUT_);};
      inline WiFiManager&       ssid             ( size_t i, std::string s ){if (i<ssidCount()) {_changed=(ssid(i)!=s); at(_SSID_).at(i) = s;}; return *this;};
      inline std::string        ssid             ( size_t i )               {if (i<ssidCount()) return at(_SSID_).at(i); return "";};
      inline WiFiManager&       reconnectionTime ( ulong v )                {_changed=(reconnectionTime()!=v); at(_TIMEOUT_) = v; return *this;};
      inline WiFiManager&       password         ( size_t i, std::string p ){if (i<ssidCount()) push_back(ssid(i), p);  return *this;};
      inline std::string        password         ( size_t i )               {if (i<ssidCount()) return at(_PWD_).at(i); return "";};
      WiFiManager&              erase            ( size_t );
      inline WiFiManager&       clear            ( void )                   {if(ssidCount()) _changed=true; at(_SSID_).clear(); disconnect(0L); return *this;};

      bool                      connect          ( void );
      inline bool               apConnected      ( void )                   {return _ap_connected;};
      inline bool               staConnected     ( void )                   {return (WiFi.status()==WL_CONNECTED);};
      inline bool               connected        ( void )                   {return (apConnected() || staConnected());};
      inline bool               disconnected     ( void )                   {return !connected();};
      WiFiManager&              disconnect       ( ulong );
      inline WiFiManager&       disconnect       ( void )                   {return disconnect(at(_TIMEOUT_));};
      void                      loop             ( void );
      inline WiFiManager&       aPModeDenied     ( void )                   {_apMode_enabled=0; return *this;};
      inline WiFiManager&       apModeAllowed    ( size_t n=-1 )            {_apMode_enabled=n; return *this;};

      void                      onConnect        ( void(*f)() )             {_on_connect=f;};
      void                      onApConnect      ( void(*f)() )             {_on_apConnect=f;};
      void                      onStaConnect     ( void(*f)() )             {_on_staConnect=f;};
      void                      onDisconnect     ( void(*f)() )             {_on_disconnect=f;};
      void                      onApDisconnect   ( void(*f)() )             {_on_apDisconnect=f;};
      void                      onStaDisconnect  ( void(*f)() )             {_on_staDisconnect=f;};
      void                      ifConnected      ( void(*f)() )             {_if_connected=f;};
      void                      ifApConnected    ( void(*f)() )             {_if_apConnected=f;};
      void                      ifStaConnected   ( void(*f)() )             {_if_staConnected=f;};
      void                      onMemoryLeak     ( void(*f)() )             {_on_memoryLeak=f;};

      bool                      saveToSD         ( void );
      bool                      restoreFromSD    ( void );

    private:
      bool                                       _enabled;
      bool                                       _ap_connected;
      size_t                                     _apMode_enabled, _trialNbr, _apTimeout;
      void                                     (*_on_connect)();
      void                                     (*_on_apConnect)();
      void                                     (*_on_staConnect)();
      void                                     (*_on_disconnect)();
      void                                     (*_on_apDisconnect)();
      void                                     (*_on_staDisconnect)();
      void                                     (*_if_connected)();
      void                                     (*_if_apConnected)();
      void                                     (*_if_staConnected)();
      void                                     (*_on_memoryLeak)();
      bool                                       _changed, _restored;
      ulong                                      _next_connect;
      size_t                                     _trial_counter, _apTimeout_counter;

      bool                                       _apConnect();
      void                                       _memoryTest();
  };
}

using namespace WiFiManagement;

#endif

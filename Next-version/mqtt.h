/* ESP8266-MQTT-Manager C++ (Version 0.1 - 2020/07)
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
// https://diyi0t.com/microcontroller-to-raspberry-pi-wifi-mqtt-communication/
// https://pubsubclient.knolleary.net/api.html
// https://links2004.github.io/Arduino/dc/da7/class_wi_fi_client.html
#ifndef HEADER_FB324C732446218
#define HEADER_FB324C732446218

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include "untyped.h"
#include "setting.h"
#include "debug.h"

// Json name attributes:
#ifndef ROUTE_MQTT_BROKER
  #define ROUTE_MQTT_BROKER       "mqttBroker"
  #define ROUTE_MQTT_PORT         "mqttPort"
  #define ROUTE_MQTT_IDENT        "mqttIdent"
  #define ROUTE_MQTT_USER         "mqttUser"
  #define ROUTE_MQTT_PWD          "mqttPwd"
  #define ROUTE_MQTT_INTOPIC      "mqttInTopic"
  #define ROUTE_MQTT_OUTOPIC      "mqttOuTopic"
#endif

namespace MQTT {
 class mqtt : public PubSubClient, public untyped {
  public:
    mqtt ( void );

    virtual ~mqtt()    {saveToSD();};

    inline mqtt&        broker         ( std::string v )      {_changed|=(at(ROUTE_MQTT_BROKER)  !=v); at(ROUTE_MQTT_BROKER)  = v; return *this;};
    inline bool         disabled       ( void )               {return broker().empty();};
    inline mqtt&        port           ( short v )            {_changed|=(at(ROUTE_MQTT_PORT)    !=v); at(ROUTE_MQTT_PORT)    = v; return *this;};
    inline mqtt&        ident          ( std::string v )      {_changed|=(at(ROUTE_MQTT_IDENT)   !=v); at(ROUTE_MQTT_IDENT)   = v; return *this;};
    inline mqtt&        user           ( std::string v )      {_changed|=(at(ROUTE_MQTT_USER)    !=v); at(ROUTE_MQTT_USER)    = v; return *this;};
    inline mqtt&        password       ( std::string v )      {_changed|=(at(ROUTE_MQTT_PWD)     !=v); at(ROUTE_MQTT_PWD)     = v; return *this;};
    inline mqtt&        inputTopic     ( std::string v )      {_changed|=(at(ROUTE_MQTT_INTOPIC) !=v); at(ROUTE_MQTT_INTOPIC) = v; return *this;};
    inline mqtt&        outputTopic    ( std::string v )      {_changed|=(at(ROUTE_MQTT_OUTOPIC)!=v);  at(ROUTE_MQTT_OUTOPIC) = v; return *this;};
    inline std::string  broker         ( void )               {return at(ROUTE_MQTT_BROKER  ).c_str();};
    inline short        port           ( void )               {return at(ROUTE_MQTT_PORT    );};
    inline std::string  ident          ( void )               {return at(ROUTE_MQTT_IDENT   ).c_str();};
    inline std::string  user           ( void )               {return at(ROUTE_MQTT_USER    ).c_str();};
    inline std::string  password       ( void )               {return at(ROUTE_MQTT_PWD     ).c_str();};
    inline std::string  inputTopic     ( void )               {return at(ROUTE_MQTT_INTOPIC ).c_str();};
    inline std::string  outputTopic    ( void )               {return at(ROUTE_MQTT_OUTOPIC).c_str();};
    inline bool         changed        ( void )               {return _changed;};
    inline mqtt&        changed        ( bool force )         {_changed=force; return *this;};
    void                loop           ( void );

    bool                reconnect      ( void );
    bool                send           ( std::string, std::string ="" );
    static void         callback       ( char*, byte*, unsigned int );

    mqtt&               set            ( untyped );
    bool                saveToSD       ( void );
    bool                restoreFromSD  ( void );

  private:
    bool                _changed;
    WiFiClient          _ethClient;

    inline static bool  _isInMqtt      ( std::string s )      {return(
          s==ROUTE_MQTT_BROKER
      ||  s==ROUTE_MQTT_PORT
      ||  s==ROUTE_MQTT_IDENT
      ||  s==ROUTE_MQTT_USER
      ||  s==ROUTE_MQTT_PWD
      ||  s==ROUTE_MQTT_INTOPIC
      ||  s==ROUTE_MQTT_OUTOPIC
    );};
 };
}
using namespace MQTT;

#endif

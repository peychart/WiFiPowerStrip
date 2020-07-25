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
#ifndef HEADER_FB324C732446218
#define HEADER_FB324C732446218

#include <Arduino.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <fstream>
#include "untyped.h"
#include "debug.h"

namespace MQTT {
// Json name attributes:
#define _MQTT_BROKER_       "broker"
#define _MQTT_PORT_         "port"
#define _MQTT_IDENT_        "ident"
#define _MQTT_USER_         "user"
#define _MQTT_PWD_          "password"
#define _MQTT_INTOPIC_      "inTopic"
#define _MQTT_OUTTOPIC_     "outTopic"

 class mqtt : public PubSubClient, public untyped
 {
  public:
    mqtt ( WiFiClient& );

    virtual ~mqtt()    {saveToSD();};

    inline mqtt&        broker         ( std::string v )      {_changed=(at(_MQTT_BROKER_)  !=v); at(_MQTT_BROKER_)   = v; return *this;};
    inline bool         enabled        ( void )               {return at(_MQTT_BROKER_).size();};
    inline bool         disabled       ( void )               {return !enabled();};
    inline mqtt&        port           ( short v )            {_changed=(at(_MQTT_PORT_)    !=v); at(_MQTT_PORT_)     = v; return *this;};
    inline mqtt&        ident          ( std::string v )      {_changed=(at(_MQTT_IDENT_)   !=v); at(_MQTT_IDENT_)    = v; return *this;};
    inline mqtt&        user           ( std::string v )      {_changed=(at(_MQTT_USER_)    !=v); at(_MQTT_USER_)     = v; return *this;};
    inline mqtt&        password       ( std::string v )      {_changed=(at(_MQTT_PWD_)     !=v); at(_MQTT_PWD_)      = v; return *this;};
    inline mqtt&        inputTopic     ( std::string v )      {_changed=(at(_MQTT_INTOPIC_) !=v); at(_MQTT_INTOPIC_)  = v; return *this;};
    inline mqtt&        outputTopic    ( std::string v )      {_changed=(at(_MQTT_OUTTOPIC_)!=v); at(_MQTT_OUTTOPIC_) = v; return *this;};
    inline std::string  broker         ( void )               {return at(_MQTT_BROKER_  ).c_str();};
    inline short        port           ( void )               {return at(_MQTT_PORT_    );};
    inline std::string  ident          ( void )               {return at(_MQTT_IDENT_   ).c_str();};
    inline std::string  user           ( void )               {return at(_MQTT_USER_    ).c_str();};
    inline std::string  password       ( void )               {return at(_MQTT_PWD_     ).c_str();};
    inline std::string  inputTopic     ( void )               {return at(_MQTT_INTOPIC_ ).c_str();};
    inline std::string  outputTopic    ( void )               {return at(_MQTT_OUTTOPIC_).c_str();};
    inline void         loop           ( void )               {if( enabled() ) PubSubClient::loop();};

    void                reconnect      ( void );
    bool                send           ( std::string, std::string="" );
    static void         callback       ( char*, byte*, unsigned int );
    bool                saveToSD       ( void );
    bool                restoreFromSD  ( void );

  private:
    bool                _changed;
 };

}

using namespace MQTT;

#endif

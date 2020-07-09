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
#ifndef HEADER_FB324C732446218
#define HEADER_FB324C732446218

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <fstream>
#include "untyped.h"
#include "debug.h"

extern untyped myConf;

namespace MQTT {
// Json name attributes:
#define _BROKER_       "broker"
#define _PORT_         "port"
#define _IDENT_        "ident"
#define _USER_         "user"
#undef  _PWD_
#define _PWD_          "password"
#define _INTOPIC_      "inTopic"
#define _OUTTOPIC_     "outTopic"

 class mqtt : public PubSubClient, private untyped
 {
  public:
    mqtt ( WiFiClient& );

    virtual ~mqtt()    {saveToSD();};

    inline mqtt&        broker         ( std::string v )              {_changed=(at(_BROKER_)  !=v); at(_BROKER_)   = v; return *this;};
    inline mqtt&        port           ( short v )                    {_changed=(at(_PORT_)    !=v); at(_PORT_)     = v; return *this;};
    inline mqtt&        ident          ( std::string v )              {_changed=(at(_IDENT_)   !=v); at(_IDENT_)    = v; return *this;};
    inline mqtt&        user           ( std::string v )              {_changed=(at(_USER_)    !=v); at(_USER_)     = v; return *this;};
    inline mqtt&        password       ( std::string v )              {_changed=(at(_PWD_)     !=v); at(_PWD_)      = v; return *this;};
    inline mqtt&        inputTopic     ( std::string v )              {_changed=(at(_INTOPIC_) !=v); at(_INTOPIC_)  = v; return *this;};
    inline mqtt&        outputTopic    ( std::string v )              {_changed=(at(_OUTTOPIC_)!=v); at(_OUTTOPIC_) = v; return *this;};
    inline std::string  broker         ( void )                       {return at(_BROKER_  );};
    inline short        port           ( void )                       {return at(_PORT_    );};
    inline std::string  ident          ( void )                       {return at(_IDENT_   );};
    inline std::string  user           ( void )                       {return at(_USER_    );};
    inline std::string  password       ( void )                       {return at(_PWD_     );};
    inline std::string  inputTopic     ( void )                       {return at(_INTOPIC_ );};
    inline std::string  outputTopic    ( void )                       {return at(_OUTTOPIC_);};

    void                reconnect      ( void );
    bool                send           ( std::string, std::string );
    static void         callback       ( char*, byte*, unsigned int );
    bool                saveToSD       ( void );
    bool                restoreFromSD  ( void );

  private:
    bool                _changed;
 };

}

using namespace MQTT;

#endif
